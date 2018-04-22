/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include "loggingfiltermodel.h"
#include "loggingmodel.h"
#include "akonadiconsole_debug.h"

#include <KCheckComboBox>

Q_DECLARE_METATYPE(LoggingModel::Message)

using namespace KPIM;

LoggingFilterModel::LoggingFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    mInvalidateTimer.setInterval(50);
    mInvalidateTimer.setSingleShot(true);
    connect(&mInvalidateTimer, &QTimer::timeout,
            this, &LoggingFilterModel::invalidate);
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
    connect(mAppFilter, &KCheckComboBox::checkedItemsChanged,
            this, [this](const QStringList &items) {
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
    connect(mTypeFilter, &KCheckComboBox::checkedItemsChanged,
            this, [this](const QStringList &items) {
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
    connect(mCategoryFilter, &KCheckComboBox::checkedItemsChanged,
            this, [this](const QStringList &items) {
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
    return mCheckedApps.contains(msg.app) && 
            mCheckedCategories.contains(msg.category) &&
            mCheckedTypes.contains(msg.type);
}
