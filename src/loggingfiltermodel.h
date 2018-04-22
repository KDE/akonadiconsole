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


#ifndef LOGGINGFILTERMODEL_H_
#define LOGGINGFILTERMODEL_H_

#include <QSortFilterProxyModel>
#include <QSet>
#include <QTimer>

namespace KPIM {
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
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    KPIM::KCheckComboBox *mAppFilter = nullptr;
    QSet<QString> mCheckedApps;
    KPIM::KCheckComboBox *mCategoryFilter = nullptr;
    QSet<QString> mCheckedCategories;
    KPIM::KCheckComboBox *mTypeFilter = nullptr;
    QSet<QtMsgType> mCheckedTypes;
    QTimer mInvalidateTimer;
};

#endif
