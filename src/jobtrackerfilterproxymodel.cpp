/*
  SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "jobtrackerfilterproxymodel.h"
#include "akonadiconsole_debug.h"
#include "jobtrackermodel.h"
#include <QDebug>
#include <QRegularExpression>

JobTrackerFilterProxyModel::JobTrackerFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(true);
}

JobTrackerFilterProxyModel::~JobTrackerFilterProxyModel() = default;

bool JobTrackerFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (mShowOnlyFailed) {
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        const QVariant var = sourceModel()->data(index, JobTrackerModel::FailedIdRole);
        if (var.isValid()) {
            bool result = var.toBool();
            if (!result) {
                return false;
            }
        }
    }
    QRegularExpression reg = filterRegularExpression();
    if (reg.pattern().isEmpty()) {
        return true;
    }
    reg.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    if (mSearchColumn == -1) { // search in all columns
        const int colCount = sourceModel()->columnCount();
        for (int i = 0; i < colCount; i++) {
            const QModelIndex index = sourceModel()->index(sourceRow, i, sourceParent);
            if (index.isValid()) {
                // qDebug() << " " << index << " data=" << sourceModel()->data(index).toString();
                if (sourceModel()->data(index).toString().contains(reg)) {
                    return true;
                }
            }
        }
        return false;
    } else {
        const int colCount = sourceModel()->columnCount();
        if (mSearchColumn < colCount) {
            const QModelIndex index = sourceModel()->index(sourceRow, mSearchColumn, sourceParent);
            return sourceModel()->data(index).toString().contains(reg);
        } else {
            qCWarning(AKONADICONSOLE_LOG) << "You try to select a column which doesn't exist " << mSearchColumn << " model number of column " << colCount;
            return true;
        }
    }
}

void JobTrackerFilterProxyModel::setShowOnlyFailed(bool showOnlyFailed)
{
    if (mShowOnlyFailed != showOnlyFailed) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
#endif
        mShowOnlyFailed = showOnlyFailed;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
        invalidateFilter();
#endif
    }
}

void JobTrackerFilterProxyModel::setSearchColumn(int column)
{
    if (mSearchColumn != column) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
#endif
        mSearchColumn = column;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
        invalidateFilter();
#endif
    }
}

#include "moc_jobtrackerfilterproxymodel.cpp"
