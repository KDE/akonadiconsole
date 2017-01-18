/*
  Copyright (c) 2017 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "jobtrackerfilterproxymodel.h"
#include <QDebug>

JobTrackerFilterProxyModel::JobTrackerFilterProxyModel(QObject *parent)
    : KRecursiveFilterProxyModel(parent),
      mSearchColumn(-1)
{
    setDynamicSortFilter(true);
}

JobTrackerFilterProxyModel::~JobTrackerFilterProxyModel()
{
}

bool JobTrackerFilterProxyModel::acceptRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QRegExp reg = filterRegExp();
    if (reg.isEmpty()) {
        return true;
    }
    reg.setCaseSensitivity(Qt::CaseInsensitive);
    if (mSearchColumn == -1) { // search in all columns
        const int colCount = sourceModel()->columnCount();
        for (int i = 0; i < colCount; i++) {
            const QModelIndex index = sourceModel()->index(sourceRow, i, sourceParent);
            if (index.isValid()) {
                //qDebug() << " " << index << " data=" << sourceModel()->data(index).toString();
                if (sourceModel()->data(index).toString().contains(reg)) {
                    return true;
                }
            }
        }
        return false;
    } else {
        const QModelIndex index = sourceModel()->index(sourceRow, mSearchColumn, sourceParent);
        return sourceModel()->data(index).toString().contains(reg);
    }
}

void JobTrackerFilterProxyModel::setSearchColumn(int column)
{
    mSearchColumn = column;
}
