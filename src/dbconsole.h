/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_DBCONSOLE_H
#define AKONADICONSOLE_DBCONSOLE_H

#include <QWidget>
#include "ui_dbconsoletab.h"

class QSqlQueryModel;
class QTabWidget;

class DbConsoleTab;

class DbConsole : public QWidget
{
    Q_OBJECT
public:
    explicit DbConsole(QWidget *parent = nullptr);

private Q_SLOTS:
    DbConsoleTab *addTab();
    void saveQueries();

private:
    int mTabCounter = 0;
    QTabWidget *mTabWidget = nullptr;
};

class DbConsoleTab : public QWidget
{
    Q_OBJECT
public:
    explicit DbConsoleTab(int index, QWidget *parent = nullptr);

    void setQuery(const QString &query);
    QString query() const;

Q_SIGNALS:
    void saveQueries();

private Q_SLOTS:
    void execClicked();
    void copyCell();

private:
    Ui::DbConsoleTab ui;
    QSqlQueryModel *mQueryModel = nullptr;
    int mIndex;
};

#endif
