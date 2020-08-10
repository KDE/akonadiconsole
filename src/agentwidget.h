/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_AGENTWIDGET_H
#define AKONADICONSOLE_AGENTWIDGET_H

#include "ui_agentwidget.h"

#include <AkonadiCore/agentinstance.h>

class KJob;
class QMenu;
class QPoint;
class QResizeEvent;
class QEvent;

class AgentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AgentWidget(QWidget *parent = nullptr);
    Akonadi::AgentInstanceWidget *widget() const
    {
        return ui.instanceWidget;
    }

protected:
    void resizeEvent(QResizeEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;
private Q_SLOTS:
    void addAgent();
    void removeAgent();
    void configureAgent();
    void configureAgentRemote();
    void synchronizeAgent();
    void synchronizeTree();
    void toggleOnline();
    void showChangeNotifications();
    void showTaskList();
    void abortAgent();
    void restartAgent();
    void slotCloneAgent();
    void cloneAgent(KJob *job);

    void currentChanged();
    void showContextMenu(const QPoint &pos);

    void selectionChanged();
    void slotDataChanged(const QModelIndex &, const QModelIndex &);

private:
    Ui::AgentWidget ui;
    QMenu *mSyncMenu, *mConfigMenu;
    Akonadi::AgentInstance mCloneSource;
};

#endif
