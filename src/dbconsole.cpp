/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbconsole.h"
#include <Akonadi/DbAccess>

#include <QAction>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KStandardAction>
#include <QIcon>

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QTabBar>
#include <QTabWidget>

DbConsole::DbConsole(QWidget *parent)
    : QWidget(parent)
{
    auto l = new QVBoxLayout(this);

    mTabWidget = new QTabWidget(this);
    mTabWidget->tabBar()->setTabsClosable(true);
    connect(mTabWidget->tabBar(), &QTabBar::tabCloseRequested, mTabWidget, [this](int index) {
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

    for (const auto &query : std::as_const(queries)) {
        auto tab = addTab();
        tab->setQuery(query);
    }
    if (queries.isEmpty()) {
        addTab();
    }
}

void DbConsole::saveQueries()
{
    QStringList queries;
    queries.reserve(mTabWidget->count());
    for (int i = 0; i < mTabWidget->count(); ++i) {
        auto tab = qobject_cast<DbConsoleTab *>(mTabWidget->widget(i));
        queries << tab->query();
    }
    KSharedConfig::openConfig()->group("DBConsole").writeEntry("queryTexts", queries);
}

DbConsoleTab *DbConsole::addTab()
{
    ++mTabCounter;
    auto tab = new DbConsoleTab(mTabCounter, mTabWidget);
    connect(tab, &DbConsoleTab::saveQueries, this, &DbConsole::saveQueries);
    mTabWidget->addTab(tab, i18n("Query #%1", mTabCounter));
    return tab;
}

DbConsoleTab::DbConsoleTab(int index, QWidget *parent)
    : QWidget(parent)
    , mIndex(index)
{
    ui.setupUi(this);

    QAction *copyAction = KStandardAction::copy(this, &DbConsoleTab::copyCell, this);
    ui.resultView->addAction(copyAction);

    ui.execButton->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));
    ui.execButton->setShortcut(Qt::CTRL | Qt::Key_Return);
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

#include "moc_dbconsole.cpp"
