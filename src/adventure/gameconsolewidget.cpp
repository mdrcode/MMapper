#include <QtCore>
#include <QtWidgets>

#include "configuration/configuration.h"
#include "gameconsolewidget.h"

GameConsoleWidget::GameConsoleWidget(AdventureJournal &aj, QWidget *parent)
    : QWidget{parent}
    , m_adventureJournal{aj}
{
    m_consoleTextEdit = new QTextEdit(this);
    m_consoleTextCursor = new QTextCursor(m_consoleTextEdit->document());

    m_consoleTextEdit->setReadOnly(true);
    m_consoleTextEdit->setOverwriteMode(true);
    m_consoleTextEdit->setUndoRedoEnabled(false);
    m_consoleTextEdit->setDocumentTitle("Game Console Text");
    m_consoleTextEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_consoleTextEdit->setTabChangesFocus(false);

    const auto &settings = getConfig().integratedClient;

    QTextFrameFormat frameFormat = m_consoleTextEdit->document()->rootFrame()->frameFormat();
    frameFormat.setBackground(settings.backgroundColor);
    m_consoleTextEdit->document()->rootFrame()->setFrameFormat(frameFormat);

    QTextCharFormat blockCharFormat = m_consoleTextCursor->blockCharFormat();
    blockCharFormat.setForeground(settings.foregroundColor);
    auto font = new QFont();
    font->fromString(settings.font); // needed fromString() to extract PointSize
    blockCharFormat.setFont(*font);
    m_consoleTextCursor->setBlockCharFormat(blockCharFormat);

    addConsoleMessage(DEFAULT_CONTENT);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_consoleTextEdit);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_killedMob,
            this,
            &GameConsoleWidget::slot_onKilledMob);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedNarrate,
            this,
            &GameConsoleWidget::slot_onReceivedNarrate);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_receivedTell,
            this,
            &GameConsoleWidget::slot_onReceivedTell);

    connect(&m_adventureJournal,
            &AdventureJournal::sig_updatedXP,
            this,
            &GameConsoleWidget::slot_onUpdatedXP);
}

void GameConsoleWidget::slot_onKilledMob(const QString &mobName)
{
    addConsoleMessage("Killed: " + mobName);
}

void GameConsoleWidget::slot_onReceivedNarrate(const QString &narr)
{
    addConsoleMessage(narr);
}

void GameConsoleWidget::slot_onReceivedTell(const QString &tell)
{
    addConsoleMessage(tell);
}

void GameConsoleWidget::slot_onUpdatedXP(const double currentXP)
{
    addConsoleMessage("Char XP updated: " + QString::number(currentXP));
}

void GameConsoleWidget::addConsoleMessage(const QString &msg)
{
    // TODO If first message, clear the placeholder text
    auto prepend = "\n";
    if (m_numMessagesReceived == 0) {
        prepend = "";
    }

    m_consoleTextCursor->movePosition(QTextCursor::End);
    m_consoleTextCursor->insertText(prepend + msg);
    m_numMessagesReceived++;

    // If more than MAX_LINES, preserve by deleting from the start
    auto lines_over = m_consoleTextEdit->document()->lineCount() - GameConsoleWidget::MAX_LINES;
    if (lines_over > 0) {
        m_consoleTextCursor->movePosition(QTextCursor::Start);
        m_consoleTextCursor->movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines_over);
        m_consoleTextCursor->removeSelectedText();
        m_consoleTextCursor->movePosition(QTextCursor::End);
    }

    // force scroll to bottom upon new message
    auto scrollBar = m_consoleTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
