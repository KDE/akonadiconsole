/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mainwindow.h"
using namespace Qt::Literals::StringLiterals;

#include "config-akonadiconsole.h"

#include "uistatesaver.h"

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>

#include <KSharedConfig>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
    , mMain(new MainWidget(this))
{
    setCentralWidget(mMain);

    KStandardActions::quit(qApp, &QApplication::quit, actionCollection());

    setupGUI(Keys /*| ToolBar | StatusBar*/ | Save | Create, u"akonadiconsoleui.rc"_s);
    AkonadiConsole::UiStateSaver::restoreState(this, KConfigGroup(KSharedConfig::openConfig(), u"UiState"_s));
    KMessageBox::information(this,
                             i18n("<p>Akonadi Console is purely a development tool. "
                                  "It allows you to view and change internal data structures of Akonadi. "
                                  "You should only change data in here if you know what you are doing, otherwise "
                                  "you risk damaging or losing your personal information management data.<br/>"
                                  "<b>Use at your own risk!</b></p>"),
                             QString(),
                             u"UseAtYourOwnRiskWarning"_s);
}

MainWindow::~MainWindow()
{
    delete mMain;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    KConfigGroup config(KSharedConfig::openConfig(), u"UiState"_s);
    AkonadiConsole::UiStateSaver::saveState(this, config);
    KSharedConfig::openConfig()->sync();
    KXmlGuiWindow::closeEvent(event);
}

#include "moc_mainwindow.cpp"
