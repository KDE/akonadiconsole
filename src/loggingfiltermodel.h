/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSet>
#include <QSortFilterProxyModel>
#include <QTimer>

namespace KPIM
{
class KCheckComboBox;
}

class LoggingFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LoggingFilterModel(QObject *parent = nullptr);
    ~LoggingFilterModel();

    void setAppFilter(KPIM::KCheckComboBox *appFilter);
    void setCategoryFilter(KPIM::KCheckComboBox *categoryFilter);
    void setTypeFilter(KPIM::KCheckComboBox *typeFilter);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    KPIM::KCheckComboBox *mAppFilter = nullptr;
    QSet<QString> mCheckedApps;
    KPIM::KCheckComboBox *mCategoryFilter = nullptr;
    QSet<QString> mCheckedCategories;
    KPIM::KCheckComboBox *mTypeFilter = nullptr;
    QSet<QtMsgType> mCheckedTypes;
    QTimer mInvalidateTimer;
};

