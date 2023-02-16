/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "monitorsmodel.h"
#include "akonadiconsole_debug.h"
#include <Akonadi/Monitor>
#include <Akonadi/Session>
#include <KLocalizedString>

#include <QTimer>

#ifndef COMPILE_WITH_UNITY_CMAKE_SUPPORT
Q_DECLARE_METATYPE(Akonadi::NotificationSubscriber)
#endif

MonitorsModel::MonitorsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    QTimer::singleShot(0, this, &MonitorsModel::init);
}

MonitorsModel::~MonitorsModel() = default;

void MonitorsModel::init()
{
    mMonitor = new Akonadi::Monitor(this);
    mMonitor->setTypeMonitored(Akonadi::Monitor::Subscribers, true);
    connect(mMonitor, &Akonadi::Monitor::notificationSubscriberAdded, this, &MonitorsModel::slotSubscriberAdded);
    connect(mMonitor, &Akonadi::Monitor::notificationSubscriberChanged, this, &MonitorsModel::slotSubscriberChanged);
    connect(mMonitor, &Akonadi::Monitor::notificationSubscriberRemoved, this, &MonitorsModel::slotSubscriberRemoved);
}

QModelIndex MonitorsModel::indexForSession(const QByteArray &session)
{
    int pos = mSessions.indexOf(session);
    if (pos == -1) {
        pos = mSessions.count();
        beginInsertRows({}, pos, pos);
        mSessions.push_back(session);
        mData.insert(session, {});
        endInsertRows();
    }

    return index(pos, 0);
}

void MonitorsModel::slotSubscriberAdded(const Akonadi::NotificationSubscriber &subscriber)
{
    auto sessionIdx = indexForSession(subscriber.sessionId());
    auto &sessions = mData[subscriber.sessionId()];
    beginInsertRows(sessionIdx, sessions.count(), sessions.count());
    sessions.push_back(subscriber);
    endInsertRows();
}

void MonitorsModel::slotSubscriberRemoved(const Akonadi::NotificationSubscriber &subscriber)
{
    int idx = -1;
    auto sessionIdx = indexForSession(subscriber.sessionId());
    auto &sessions = mData[subscriber.sessionId()];
    for (auto it = sessions.begin(), end = sessions.end(); it != end; ++it) {
        ++idx;
        if (it->subscriber() == subscriber.subscriber()) {
            beginRemoveRows(sessionIdx, idx, idx);
            sessions.erase(it);
            endRemoveRows();
            return;
        }
    }
}

void MonitorsModel::slotSubscriberChanged(const Akonadi::NotificationSubscriber &subscriber)
{
    int row = -1;
    auto sessionIdx = indexForSession(subscriber.sessionId());
    auto sessions = mData[subscriber.sessionId()];
    for (auto it = sessions.begin(), end = sessions.end(); it != end; ++it) {
        ++row;
        if (it->subscriber() == subscriber.subscriber()) {
            *it = subscriber;
            const auto idx = index(row, 0, sessionIdx);
            Q_EMIT dataChanged(idx, idx);
            return;
        }
    }
}

QVariant MonitorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            if (section == 0) {
                return i18n("Session/Subscriber");
            }
        }
    }

    return {};
}

QVariant MonitorsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0) {
        return {};
    }
    if ((int)index.internalId() == -1) {
        if (index.row() >= mSessions.count()) {
            return {};
        }

        if (role == Qt::DisplayRole) {
            return mSessions.at(index.row());
        }
    } else {
        const auto session = mSessions.at(index.parent().row());
        const auto subscribers = mData.value(session);
        if (index.row() >= subscribers.count()) {
            return {};
        }
        const auto subscriber = subscribers.at(index.row());
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            return subscriber.subscriber();
        } else if (role == SubscriberRole) {
            return QVariant::fromValue(subscriber);
        }
    }

    return {};
}

int MonitorsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

int MonitorsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mSessions.count();
    }

    if ((int)parent.internalId() == -1) {
        const auto session = mSessions.at(parent.row());
        return mData.value(session).count();
    }

    return 0;
}

QModelIndex MonitorsModel::parent(const QModelIndex &child) const
{
    if ((int)child.internalId() == -1) {
        return {};
    } else {
        return index(child.internalId(), 0, {});
    }

    return {};
}

QModelIndex MonitorsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, -1);
    }

    return createIndex(row, column, parent.row());
}
