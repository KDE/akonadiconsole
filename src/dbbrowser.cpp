/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbbrowser.h"
#include "dbaccess.h"

#include <QSqlTableModel>

#include <QIcon>

DbBrowser::DbBrowser(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    if (DbAccess::database().isOpen()) {
        QStringList userTables = DbAccess::database().tables(QSql::Tables);
        userTables.sort();
        QStringList systemTables = DbAccess::database().tables(QSql::SystemTables);
        systemTables.sort();

        ui.tableBox->addItems(QStringList() << userTables << systemTables);
    }

    ui.refreshButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    connect(ui.refreshButton, &QPushButton::clicked, this, &DbBrowser::refreshClicked);
}

void DbBrowser::refreshClicked()
{
    const QString table = ui.tableBox->currentText();
    if (table.isEmpty()) {
        return;
    }
    delete mTableModel;
    mTableModel = new QSqlTableModel(this, DbAccess::database());
    mTableModel->setTable(table);
    mTableModel->setEditStrategy(QSqlTableModel::OnRowChange);
    mTableModel->select();
    ui.tableView->setModel(mTableModel);
    connect(ui.tableView->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &DbBrowser::onSortIndicatorChanged);
}

void DbBrowser::onSortIndicatorChanged(int column, Qt::SortOrder order)
{
    mTableModel->sort(column, order);
}
