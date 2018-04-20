/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef MONITORSMODEL_H
#define MONITORSMODEL_H

#include <QAbstractItemModel>
#include <AkonadiCore/NotificationSubscriber>
#include "libakonadiconsole_export.h"

namespace Akonadi
{
class Monitor;
}

class LIBAKONADICONSOLE_EXPORT MonitorsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        SubscriberRole = Qt::UserRole
    };
        
    explicit MonitorsModel(QObject *parent = nullptr);
    virtual ~MonitorsModel();

    void setEnabled(bool enabled);

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

private Q_SLOTS:
    void init();

    void slotSubscriberAdded(const Akonadi::NotificationSubscriber &subscriber);
    void slotSubscriberChanged(const Akonadi::NotificationSubscriber &subscriber);
    void slotSubscriberRemoved(const Akonadi::NotificationSubscriber &subscriber);

    QModelIndex indexForSession(const QByteArray &sesion);

private:
    QList<QByteArray> mSessions;
    QHash<QByteArray /* session */, QVector<Akonadi::NotificationSubscriber>> mData;
    Akonadi::Monitor *mMonitor;
};

#endif // MONITORSMODEL_H
