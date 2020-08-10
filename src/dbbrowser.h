/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_DBBROWSER_H
#define AKONADICONSOLE_DBBROWSER_H

#include "ui_dbbrowser.h"

class QSqlTableModel;

class DbBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit DbBrowser(QWidget *parent = nullptr);

private Q_SLOTS:
    void refreshClicked();
    void onSortIndicatorChanged(int column, Qt::SortOrder order);

private:
    Ui::DbBrowser ui;
    QSqlTableModel *mTableModel = nullptr;
};

#endif
