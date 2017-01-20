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

#ifndef JOBTRACKERFILTERPROXYMODEL_H
#define JOBTRACKERFILTERPROXYMODEL_H

#include <KRecursiveFilterProxyModel>

class JobTrackerFilterProxyModel : public KRecursiveFilterProxyModel
{
    Q_OBJECT
public:
    explicit JobTrackerFilterProxyModel(QObject *parent = nullptr);
    ~JobTrackerFilterProxyModel();

    void setSearchColumn(int column);

    void setShowOnlyFailed(bool showOnlyFailed);

protected:
    bool acceptRow(int source_row, const QModelIndex &source_parent) const Q_DECL_OVERRIDE;

private:
    int mSearchColumn;
    bool mShowOnlyFailed;
};

#endif // JOBTRACKERFILTERPROXYMODEL_H
