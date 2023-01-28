#include "charstatus.h"
#include "QtCore/qdebug.h"

AdventureProgress::AdventureProgress(QString charName)
    : m_CharName{charName}
    , m_xpInitial{0.0}
    , m_xpCheckpoint{0.0}
    , m_xpCurrent{0.0}
{}

QString AdventureProgress::name() const
{
    return m_CharName;
}

double AdventureProgress::xpInitial() const
{
    return m_xpInitial;
}

double AdventureProgress::xpCurrent() const
{
    return m_xpCurrent;
}

double AdventureProgress::checkpointXP()
{
    double xpGained = m_xpCurrent - m_xpCheckpoint;
    m_xpCheckpoint = m_xpCurrent;

    return xpGained;
}

void AdventureProgress::updateXP(double xpCurrent)
{
    if (m_xpInitial == 0.0) {
        qDebug().noquote() << QString("Adventure: initial XP: %1")
                                  .arg(QString::number(xpCurrent, 'f', 0));

        m_xpInitial = xpCurrent;
        m_xpCheckpoint = xpCurrent;
    }

    m_xpCurrent = xpCurrent;
}
