/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "loggingfiltermodel.h"
#include "akonadiconsole_debug.h"
#include "loggingmodel.h"

#include <KCheckComboBox>
#ifndef COMPILE_WITH_UNITY_CMAKE_SUPPORT
Q_DECLARE_METATYPE(LoggingModel::Message)
#endif

using namespace KPIM;

LoggingFilterModel::LoggingFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    mInvalidateTimer.setInterval(50);
    mInvalidateTimer.setSingleShot(true);
    connect(&mInvalidateTimer, &QTimer::timeout, this, &LoggingFilterModel::invalidate);
}

LoggingFilterModel::~LoggingFilterModel()
{
}

void LoggingFilterModel::setAppFilter(KCheckComboBox *appFilter)
{
    if (mAppFilter) {
        mAppFilter->disconnect(this);
    }
    mAppFilter = appFilter;
    connect(mAppFilter, &KCheckComboBox::checkedItemsChanged, this, [this](const QStringList &items) {
        mCheckedApps.clear();
        mCheckedApps.reserve(items.count());
        for (const auto &item : items) {
            mCheckedApps.insert(item);
        }
        if (!mInvalidateTimer.isActive()) {
            mInvalidateTimer.start();
        }
    });
}

void LoggingFilterModel::setTypeFilter(KCheckComboBox *typeFilter)
{
    if (mTypeFilter) {
        mTypeFilter->disconnect(this);
    }
    mTypeFilter = typeFilter;
    connect(mTypeFilter, &KCheckComboBox::checkedItemsChanged, this, [this](const QStringList &items) {
        mCheckedTypes.clear();
        mCheckedTypes.reserve(items.count());
        for (const auto &item : mTypeFilter->checkedItems(Qt::UserRole)) {
            mCheckedTypes.insert(static_cast<QtMsgType>(item.toInt()));
        }
        if (!mInvalidateTimer.isActive()) {
            mInvalidateTimer.start();
        }
    });
}

void LoggingFilterModel::setCategoryFilter(KCheckComboBox *categoryFilter)
{
    if (mCategoryFilter) {
        mCategoryFilter->disconnect(this);
    }
    mCategoryFilter = categoryFilter;
    connect(mCategoryFilter, &KCheckComboBox::checkedItemsChanged, this, [this](const QStringList &items) {
        mCheckedCategories.clear();
        mCheckedCategories.reserve(items.count());
        for (const auto &item : items) {
            mCheckedCategories.insert(item);
        }
        if (!mInvalidateTimer.isActive()) {
            mInvalidateTimer.start();
        }
    });
}

bool LoggingFilterModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    const auto source_idx = sourceModel()->index(source_row, 0);
    const auto msg = sourceModel()->data(source_idx, LoggingModel::MessageRole).value<LoggingModel::Message>();
    return mCheckedApps.contains(msg.app) && mCheckedCategories.contains(msg.category) && mCheckedTypes.contains(msg.type);
}
