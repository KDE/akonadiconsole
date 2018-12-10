/*
 * Copyright 2018  David Faure <faure@kde.org>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NOTIFICATIONFILTERMODEL_H
#define NOTIFICATIONFILTERMODEL_H

#include <QSortFilterProxyModel>

#include <AkonadiCore/ChangeNotification>

#include <QTimer>
#include <QSet>

namespace KPIM {
class KCheckComboBox;
}

/**
 * Filtering for NotificationModel
 */
class NotificationFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit NotificationFilterModel(QObject *parent = nullptr);

    ~NotificationFilterModel();

    void setTypeFilter(KPIM::KCheckComboBox *typeFilter);

    void setSourceModel(QAbstractItemModel *model) override;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    void slotRowsInserted(const QModelIndex &source_parent, int start, int end);

    KPIM::KCheckComboBox *mTypeFilter = nullptr;
    QSet<Akonadi::ChangeNotification::Type> mCheckedTypes;

    QSet<QString> mTypes;
    QTimer mInvalidateTimer;
};

#endif // NOTIFICATIONFILTERMODEL_H
