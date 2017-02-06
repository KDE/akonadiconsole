/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "dbconsole.h"
#include "dbaccess.h"

#include <QAction>

#include <KStandardAction>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QIcon>

#include <QApplication>
#include <QClipboard>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QFontDatabase>
#include <QTabWidget>
#include <QTabBar>

DbConsole::DbConsole(QWidget *parent)
    : QWidget(parent)
    , mTabCounter(0)
{
    auto l = new QVBoxLayout(this);

    mTabWidget = new QTabWidget(this);
    mTabWidget->tabBar()->setTabsClosable(true);
    connect(mTabWidget->tabBar(), &QTabBar::tabCloseRequested,
            mTabWidget, [this](int index) {
                mTabWidget->removeTab(index);
                saveQueries();
            });
    l->addWidget(mTabWidget);

    auto addTabButton = new QPushButton(QIcon::fromTheme(QStringLiteral("tab-new")), QString());
    connect(addTabButton, &QPushButton::clicked, this, &DbConsole::addTab);
    mTabWidget->setCornerWidget(addTabButton, Qt::TopRightCorner);

    auto config = KSharedConfig::openConfig();
    auto group = config->group("DBConsole");
    QStringList queries;
    const QString queryText = group.readEntry("queryText");
    if (!queryText.isEmpty()) {
        queries << queryText;
        group.deleteEntry("queryText");
    } else {
        queries = group.readEntry("queryTexts", QStringList());
    }

    for (const auto &query : queries) {
        auto tab = addTab();
        tab->setQuery(query);
    }
}

void DbConsole::saveQueries()
{
    QStringList queries;
    queries.reserve(mTabWidget->count());
    for (int i = 0; i < mTabWidget->count(); ++i) {
        auto tab = qobject_cast<DbConsoleTab*>(mTabWidget->widget(i));
        queries << tab->query();
    }
    KSharedConfig::openConfig()->group("DBConsole").writeEntry("queryTexts", queries);
}

DbConsoleTab *DbConsole::addTab()
{
    ++mTabCounter;
    auto tab = new DbConsoleTab(mTabCounter, mTabWidget);
    connect(tab, &DbConsoleTab::saveQueries, this, &DbConsole::saveQueries);
    mTabWidget->addTab(tab, QStringLiteral("Query #%1").arg(mTabCounter));
    return tab;
}

DbConsoleTab::DbConsoleTab(int index, QWidget *parent) :
    QWidget(parent),
    mQueryModel(nullptr),
    mIndex(index)
{
    ui.setupUi(this);

    QAction *copyAction = KStandardAction::copy(this, &DbConsoleTab::copyCell, this);
    ui.resultView->addAction(copyAction);

    ui.execButton->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));
    ui.execButton->setShortcut(Qt::CTRL + Qt::Key_Return);
    connect(ui.execButton, &QPushButton::clicked, this, &DbConsoleTab::execClicked);

    ui.queryEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui.errorView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void DbConsoleTab::setQuery(const QString &query)
{
    ui.queryEdit->setPlainText(query);
}

QString DbConsoleTab::query() const
{
    return ui.queryEdit->toPlainText();
}

void DbConsoleTab::execClicked()
{
    const QString query = ui.queryEdit->toPlainText();
    if (query.isEmpty()) {
        return;
    }
    delete mQueryModel;
    mQueryModel = new QSqlQueryModel(this);
    mQueryModel->setQuery(query, DbAccess::database());
    ui.resultView->setModel(mQueryModel);

    if (mQueryModel->lastError().isValid()) {
        ui.errorView->appendPlainText(mQueryModel->lastError().text());
        ui.resultStack->setCurrentWidget(ui.errorViewPage);
    } else {
        ui.errorView->clear();
        ui.resultStack->setCurrentWidget(ui.resultViewPage);
    }

    Q_EMIT saveQueries();
}

void DbConsoleTab::copyCell()
{
    QModelIndex index = ui.resultView->currentIndex();
    if (!index.isValid()) {
        return;
    }
    QString text = index.data(Qt::DisplayRole).toString();
    QApplication::clipboard()->setText(text);
}

