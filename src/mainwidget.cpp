/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mainwidget.h"

#include "config-akonadiconsole.h"
#include "agentwidget.h"
#include "dbbrowser.h"
#include "dbconsole.h"
#include "debugwidget.h"
#ifdef ENABLE_SEARCH
#include "searchwidget.h"
#endif
#include "jobtrackerwidget.h"
#include "notificationmonitor.h"
#include "monitorswidget.h"
#include "querydebugger.h"
#include "logging.h"

#include <AkonadiWidgets/agentinstancewidget.h>
#include <AkonadiCore/agentfilterproxymodel.h>
#include <AkonadiWidgets/controlgui.h>
#include <AkonadiCore/searchcreatejob.h>
#include <AkonadiCore/servermanager.h>

#include <QAction>
#include <KActionCollection>
#include <QTabWidget>
#include <KXmlGuiWindow>

#include <QVBoxLayout>

MainWidget::MainWidget(KXmlGuiWindow *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("mainTab"));
    layout->addWidget(tabWidget);

    tabWidget->addTab(new AgentWidget(tabWidget), QStringLiteral("Agents"));
    mBrowser = new BrowserWidget(parent, tabWidget);
    tabWidget->addTab(mBrowser, QStringLiteral("Browser"));
    tabWidget->addTab(new DebugWidget(tabWidget), QStringLiteral("Debugger"));
    tabWidget->addTab(new Logging(tabWidget), QStringLiteral("Logging"));
    tabWidget->addTab(new DbBrowser(tabWidget), QStringLiteral("DB Browser"));
    tabWidget->addTab(new DbConsole(tabWidget), QStringLiteral("DB Console"));
    tabWidget->addTab(new QueryDebugger(tabWidget), QStringLiteral("Query Debugger"));
    tabWidget->addTab(new JobTrackerWidget("jobtracker", tabWidget, QStringLiteral("Enable job tracker")), QStringLiteral("Job Tracker"));
    tabWidget->addTab(new JobTrackerWidget("resourcesJobtracker", tabWidget, QStringLiteral("Enable tracking of Resource Schedulers")), QStringLiteral("Resources Schedulers"));
    tabWidget->addTab(new NotificationMonitor(tabWidget), QStringLiteral("Notification Monitor"));
#ifdef ENABLE_SEARCH
    tabWidget->addTab(new SearchWidget(tabWidget), QStringLiteral("Item Search"));
#endif
    tabWidget->addTab(new MonitorsWidget(tabWidget), QStringLiteral("Monitors"));

    auto action = parent->actionCollection()->addAction(QStringLiteral("akonadiconsole_akonadi2xml"));
    action->setText(QStringLiteral("Dump to XML..."));
    connect(action, &QAction::triggered, mBrowser, &BrowserWidget::dumpToXml);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiconsole_clearcache"));
    action->setText(QStringLiteral("Clear Akonadi Cache"));
    connect(action, &QAction::triggered, mBrowser, &BrowserWidget::clearCache);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiserver_start"));
    action->setText(QStringLiteral("Start Server"));
    connect(action, &QAction::triggered, this, &MainWidget::startServer);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiserver_stop"));
    action->setText(QStringLiteral("Stop Server"));
    connect(action, &QAction::triggered, this, &MainWidget::stopServer);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiserver_restart"));
    action->setText(QStringLiteral("Restart Server"));
    connect(action, &QAction::triggered, this, &MainWidget::restartServer);
}

MainWidget::~MainWidget()
{
    delete mBrowser;
}

void MainWidget::startServer()
{
    Akonadi::ServerManager::start();
}

void MainWidget::stopServer()
{
    Akonadi::ServerManager::stop();
}

void MainWidget::restartServer()
{
    Akonadi::ControlGui::restart(this);
}
