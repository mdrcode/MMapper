#include <QJsonDocument>
#include <QJsonObject>

#include "adventuretracker.h"
#include "parser/parserutils.h"

#include <QDebug>

AdventureTracker::AdventureTracker(GameObserver &observer, QObject *const parent)
    : QObject{parent}
    , m_observer{observer}
{
    connect(&m_observer,
            &GameObserver::sig_sentToUserText,
            this,
            &AdventureTracker::slot_onUserText);

    connect(&m_observer,
            &GameObserver::sig_sentToUserGmcp,
            this,
            &AdventureTracker::slot_onUserGmcp);
}

AdventureTracker::~AdventureTracker() {}

void AdventureTracker::slot_onUserText(const QByteArray &ba)
{
    QString str = QString::fromLatin1(ba).trimmed();
    ParserUtils::removeAnsiMarksInPlace(str);

    // We keep track of the N most recent lines so that we can parse with the
    // context of a sliding window. Array indices are interpeted backwards,
    // so [0] is the last line received, [1] is the second-to-last, etc.
    // Must create our own QString copy for this, since the `ba` passed above
    // is reused.
    for (size_t i = m_lastLines.size(); i > 0; i--) {
        m_lastLines[i] = m_lastLines[i - 1]; // shift the array
    }
    m_lastLines[0] = new QString(str); // add a copy of the new line

    parseIfKillAndXP();

    parseIfAchievedSomething();

    parseIfReceivedHint();

    parseIfGainedALevel();
}

void AdventureTracker::slot_onUserGmcp(const GmcpMessage &msg)
{
    // https://mume.org/help/generic_mud_communication_protocol

    if (!(msg.isCharName() or msg.isCharStatusVars() or msg.isCharVitals()
          or msg.isCommChannelText()))
        return;

    auto s = msg.getJson()->toQString().toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(s);

    if (!doc.isObject()) {
        qInfo() << "Received GMCP: " << msg.getName().toQString()
                << "containing invalid Json: expecting object, got: " << s;
        return;
    }

    parseIfReceivedComm(msg, doc);

    parseIfUpdatedXP(msg, doc);

    parseIfUpdatedChar(msg, doc);
}

void AdventureTracker::parseIfAchievedSomething()
{
    // If second most recent line starts with "You achieved something new!" then
    // we interpet the most recent line as an achivement. Hopefully this will be
    // replaced with GMCP at some point.
    auto line2 = m_lastLines[1];

    if (line2 == nullptr or line2->indexOf("You achieved something new!") != 0)
        return;

    // So, 2nd-to-last line starts with You achieved, achievement is last line
    auto achievement = m_lastLines[0];
    if (achievement == nullptr)
        return;

    double xpGained = checkpointXP();
    emit sig_achievedSomething(*achievement, xpGained);
}

void AdventureTracker::parseIfGainedALevel()
{
    auto lastLine = m_lastLines[0];
    if (lastLine == nullptr or lastLine->indexOf("You gain a level!") != 0)
        return;

    emit sig_gainedLevel();
}

void AdventureTracker::parseIfKillAndXP()
{
    // We define a "kill with XP earned" event as:
    //   - The last line received contains "is dead! R.I.P." or "disappears into nothing."
    //   - AND among the K most recent lines previously received, at least one contains:
    //     -  "You receive your share of experience."
    //     -  "You gain a level!
    //     -  "You feel revitalized as the dark power within" (Dark Oath)

    auto lastLine = m_lastLines[0];

    if (lastLine == nullptr)
        return;

    auto idx_dead = lastLine->indexOf(" is dead! R.I.P.");
    if (idx_dead == -1)
        lastLine->indexOf(" disappears into nothing.");
    if (idx_dead == -1)
        return;

    // A kill has occurred, but did the player earn xp?
    auto mobName = lastLine->left(idx_dead);
    bool earnedXP = std::any_of(m_lastLines.begin(), m_lastLines.end(), [](QString *l) {
        return l != nullptr
               and (l->contains("You receive your share of experience.")
                    or l->contains("You gain a level!")
                    or l->contains("You feel revitalized as the dark power within"));
    });

    if (!earnedXP)
        return;

    double xpGained = checkpointXP();
    emit sig_killedMob(mobName, xpGained);
}

void AdventureTracker::parseIfReceivedComm(GmcpMessage msg, QJsonDocument doc)
{
    QJsonObject obj = doc.object();

    if (!msg.isCommChannelText() or !obj.contains("channel") or !obj.contains("text"))
        return;

    if (obj["channel"].toString() == "tells") {
        emit sig_receivedTell(obj["text"].toString());

    } else if (obj["channel"].toString() == "tales") {
        emit sig_receivedNarrate(obj["text"].toString());
    }
}

void AdventureTracker::parseIfReceivedHint()
{
    // If second most recent line starts with "# Hint:" then we interpet the
    // most recent line as an achivement.
    auto line2 = m_lastLines[1];
    if (line2 == nullptr or line2->indexOf("# Hint:") != 0)
        return;

    // So most recent line is hint text. Skip the leading "#  "
    auto hint = m_lastLines[0]->mid(4);
    emit sig_receivedHint(hint);
}

void AdventureTracker::parseIfUpdatedChar(GmcpMessage msg, QJsonDocument doc)
{
    QJsonObject obj = doc.object();

    if (obj.contains("name")) {
        updateCharfromMud(obj["name"].toString());
    }
}

void AdventureTracker::parseIfUpdatedXP(GmcpMessage msg, QJsonDocument doc)
{
    QJsonObject obj = doc.object();

    if (obj.contains("xp")) {
        updateXPfromMud(obj["xp"].toDouble());
    }
}

void AdventureTracker::updateCharfromMud(QString charName)
{
    if (!m_currentCharName.has_value()) {
        qDebug().noquote() << QString("Adventure: new session for char %1").arg(charName);
        m_currentCharName = charName;
        return;
    }

    if (charName == m_currentCharName.value()) {
        // nothing to do here, same character
        return;
    }

    // So a new character has logged in, need to wipe the old state
    qDebug().noquote() << QString("Adventure: char change, %1 replacing %2")
                              .arg(charName)
                              .arg(m_currentCharName.value());

    m_currentCharName = charName;
    m_xpInitial.reset();
    m_xpCheckpoint.reset();
    m_xpCurrent.reset();
}

void AdventureTracker::updateXPfromMud(double currentXP)
{
    if (!m_xpInitial.has_value()) {
        qDebug().noquote() << "Adventure: initial XP: " + QString::number(currentXP, 'f', 0);
        m_xpInitial = currentXP;
    }

    if (!m_xpCheckpoint.has_value()) {
        m_xpCheckpoint = currentXP;
    }

    m_xpCurrent = currentXP;
    emit sig_updatedXP(m_xpCurrent.value());
}

double AdventureTracker::checkpointXP()
{
    if (!m_xpCurrent.has_value() or !m_xpCheckpoint.has_value()) {
        qDebug().noquote() << "Adventure: attempting to checkpointXP() without valid state.";
        return 0;
    }

    double xpGained = m_xpCurrent.value() - m_xpCheckpoint.value();
    m_xpCheckpoint.emplace(m_xpCurrent.value());

    return xpGained;
}
