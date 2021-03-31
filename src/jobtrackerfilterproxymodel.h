/*
  SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QSortFilterProxyModel>

class JobTrackerFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit JobTrackerFilterProxyModel(QObject *parent = nullptr);
    ~JobTrackerFilterProxyModel();

    void setSearchColumn(int column);

    void setShowOnlyFailed(bool showOnlyFailed);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    int mSearchColumn = -1;
    bool mShowOnlyFailed = false;
};

