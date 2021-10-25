/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#pragma once

#include "libakonadiconsole_export.h"
#include <Akonadi/NotificationSubscriber>
#include <QAbstractItemModel>

namespace Akonadi
{
class Monitor;
}

class LIBAKONADICONSOLE_EXPORT MonitorsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles { SubscriberRole = Qt::UserRole };

    explicit MonitorsModel(QObject *parent = nullptr);
    ~MonitorsModel() override;

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

