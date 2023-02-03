#pragma once

#include "adventure/adventuretracker.h"
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>

class XPStatusWidget : public QPushButton
{
    Q_OBJECT
public:
    XPStatusWidget(AdventureTracker &at, QStatusBar *sb = nullptr, QWidget *parent = nullptr);

public slots:
    void slot_updatedSession(const AdventureSession &session);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    double calculateHourlyRateXP();
    void updateContent();

    QStatusBar *m_statusBar;

    AdventureTracker &m_tracker;

    std::optional<AdventureSession> m_session;
};
