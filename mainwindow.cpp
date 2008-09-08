/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "mainwindow.h"

#include "mainwidget.h"
#include "uistatesaver.h"

#include <akonadi/control.h>

#include <KAction>
#include <KActionCollection>
#include <KConfigGroup>
#include <KLocale>
#include <KStandardAction>
#include <QApplication>

MainWindow::MainWindow( QWidget *parent )
  : KXmlGuiWindow( parent )
{
  Akonadi::Control::start( this );
  setCentralWidget( new MainWidget( this ) );

  KStandardAction::quit( qApp, SLOT(quit()), actionCollection() );

  setupGUI( Keys /*| ToolBar | StatusBar*/ | Save | Create, "akonadiconsoleui.rc" );

  KPIM::UiStateSaver::restoreState( this, KConfigGroup( KGlobal::config(), "UiState" ) );
}

bool MainWindow::queryExit()
{
  KConfigGroup config( KGlobal::config(), "UiState" );
  KPIM::UiStateSaver::saveState( this, config );
  return KXmlGuiWindow::queryExit();
}
