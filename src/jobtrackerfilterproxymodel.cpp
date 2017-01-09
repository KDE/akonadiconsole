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

JobTrackerFilterProxyModel::JobTrackerFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent),
      mSearchColumn(0)
{

}

JobTrackerFilterProxyModel::~JobTrackerFilterProxyModel()
{

}

bool JobTrackerFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (filterRegExp().isEmpty()) {
        return true;
    }
    QModelIndex index = sourceModel()->index(sourceRow, mSearchColumn, sourceParent);
    QRegExp reg = filterRegExp();
    reg.setCaseSensitivity(Qt::CaseInsensitive);
    return sourceModel()->data(index).toString().contains(reg);
}

void JobTrackerFilterProxyModel::setSearchColumn(int column)
{
    mSearchColumn = column;
}
