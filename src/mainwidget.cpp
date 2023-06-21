/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mainwidget.h"

#include "agentwidget.h"
#include "config-akonadiconsole.h"
#include "dbbrowser.h"
#include "dbconsole.h"
#include "debugwidget.h"
#if ENABLE_SEARCH
#include "searchwidget.h"
#endif
#include "jobtrackerwidget.h"
#include "logging.h"
#include "monitorswidget.h"
#include "notificationmonitor.h"
#include "querydebugger.h"

#include <Akonadi/AgentFilterProxyModel>
#include <Akonadi/AgentInstanceWidget>
#include <Akonadi/ControlGui>
#include <Akonadi/SearchCreateJob>
#include <Akonadi/ServerManager>

#include <KActionCollection>
#include <KLocalizedString>
#include <KXmlGuiWindow>
#include <QAction>
#include <QTabWidget>

#include <QVBoxLayout>

MainWidget::MainWidget(KXmlGuiWindow *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);

    auto tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("mainTab"));
    layout->addWidget(tabWidget);

    tabWidget->addTab(new AgentWidget(tabWidget), i18n("Agents"));
    mBrowser = new BrowserWidget(parent, tabWidget);
    tabWidget->addTab(mBrowser, i18n("Browser"));
    tabWidget->addTab(new DebugWidget(tabWidget), i18n("Debugger"));
    tabWidget->addTab(new Logging(tabWidget), i18n("Logging"));
    tabWidget->addTab(new DbBrowser(tabWidget), i18n("DB Browser"));
    tabWidget->addTab(new DbConsole(tabWidget), i18n("DB Console"));
    tabWidget->addTab(new QueryDebugger(tabWidget), i18n("Query Debugger"));
    tabWidget->addTab(new JobTrackerWidget("jobtracker", tabWidget, i18n("Enable job tracker")), i18n("Job Tracker"));
    tabWidget->addTab(new JobTrackerWidget("resourcesJobtracker", tabWidget, i18n("Enable tracking of Resource Schedulers")), i18n("Resources Schedulers"));
    tabWidget->addTab(new NotificationMonitor(tabWidget), i18n("Notification Monitor"));
#if ENABLE_SEARCH
    tabWidget->addTab(new SearchWidget(tabWidget), i18n("Item Search"));
#endif
    tabWidget->addTab(new MonitorsWidget(tabWidget), i18n("Monitors"));

    auto action = parent->actionCollection()->addAction(QStringLiteral("akonadiconsole_akonadi2xml"));
    action->setText(i18n("Dump to XML..."));
    connect(action, &QAction::triggered, mBrowser, &BrowserWidget::dumpToXml);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiconsole_clearcache"));
    action->setText(i18n("Clear Akonadi Cache"));
    connect(action, &QAction::triggered, mBrowser, &BrowserWidget::clearCache);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiserver_start"));
    action->setText(i18n("Start Server"));
    connect(action, &QAction::triggered, this, &MainWidget::startServer);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiserver_stop"));
    action->setText(i18n("Stop Server"));
    connect(action, &QAction::triggered, this, &MainWidget::stopServer);

    action = parent->actionCollection()->addAction(QStringLiteral("akonadiserver_restart"));
    action->setText(i18n("Restart Server"));
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

#include "moc_mainwidget.cpp"
