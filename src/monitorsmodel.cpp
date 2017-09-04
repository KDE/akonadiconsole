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

#include "monitorsmodel.h"
#include "akonadiconsole_debug.h"
#include <AkonadiCore/Monitor>
#include <AkonadiCore/NotificationSubscriber>
#include <AkonadiCore/Session>

#include <QTimer>

MonitorsModel::MonitorsModel(QObject *parent):
    QAbstractItemModel(parent),
    mMonitor(nullptr)
{
    QTimer::singleShot(0, this, &MonitorsModel::init);
}

MonitorsModel::~MonitorsModel()
{
}

void MonitorsModel::init()
{
    mMonitor = new Akonadi::Monitor(this);
    mMonitor->setTypeMonitored(Akonadi::Monitor::Subscribers, true);
    connect(mMonitor, &Akonadi::Monitor::notificationSubscriberAdded,
            this, &MonitorsModel::slotSubscriberAdded);
    connect(mMonitor, &Akonadi::Monitor::notificationSubscriberChanged,
            this, &MonitorsModel::slotSubscriberChanged);
    connect(mMonitor, &Akonadi::Monitor::notificationSubscriberRemoved,
            this, &MonitorsModel::slotSubscriberRemoved);
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
    int idx = -1;
    auto sessionIdx = indexForSession(subscriber.sessionId());
    auto sessions = mData[subscriber.sessionId()];
    for (auto it = sessions.begin(), end = sessions.end(); it != end; ++it) {
        ++idx;
        if (it->subscriber() == subscriber.subscriber()) {
            *it = subscriber;
            Q_EMIT dataChanged(index(idx, 0, sessionIdx), index(idx, ColumnsCount, sessionIdx));
            return;
        }
    }
}

QVariant MonitorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (static_cast<Column>(section)) {
            case IdentifierColumn: return QStringLiteral("Session/Subscriber");
            case IsAllMonitoredColumn: return QStringLiteral("Monitor All");
            case MonitoredCollectionsColumn: return QStringLiteral("Collections");
            case MonitoredItemsColumn: return QStringLiteral("Items");
            case MonitoredTagsColumn: return QStringLiteral("Tags");
            case MonitoredResourcesColumn: return QStringLiteral("Resources");
            case MonitoredMimeTypesColumn: return QStringLiteral("Mime Types");
            case MonitoredTypesColumn: return QStringLiteral("Types");
            case IgnoredSessionsColumn: return QStringLiteral("Ignored Sessions");
            case IsExclusiveColumn: return QStringLiteral("Exclusive");
            case ColumnsCount: Q_ASSERT(false); return QString();
            }
        }
    }

    return QVariant();
}

namespace
{

template<typename T>
QString toString(const QSet<T> &set)
{
    QStringList rv;
    for (const auto &v : set) {
        rv << QVariant(v).toString();
    }
    return rv.join(QStringLiteral(", "));
}

template<>
QString toString(const QSet<Akonadi::Monitor::Type> &set)
{
    QStringList rv;
    for (auto v : set) {
        switch (v) {
        case Akonadi::Monitor::Items:
            rv << QStringLiteral("Items");
            break;
        case Akonadi::Monitor::Collections:
            rv << QStringLiteral("Collections");
            break;
        case Akonadi::Monitor::Tags:
            rv << QStringLiteral("Tags");
            break;
        case Akonadi::Monitor::Relations:
            rv << QStringLiteral("Relations");
            break;
        case Akonadi::Monitor::Subscribers:
            rv << QStringLiteral("Subscribers");
            break;
        case Akonadi::Monitor::Notifications:
            rv << QStringLiteral("Debug Notifications");
            break;
        }
    }
    return rv.join(QStringLiteral(", "));
}

}

QVariant MonitorsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if ((int)index.internalId() == -1) {
        if (index.row() >= mSessions.count()) {
            return {};
        }
        
        if (role == Qt::DisplayRole && index.column() == 0) {
            return mSessions.at(index.row());
        }
    } else {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            const auto session = mSessions.at(index.parent().row());
            const auto subscribers = mData.value(session);
            if (index.row() >= subscribers.count()) {
                return {};
            }

            const auto subscriber = subscribers.at(index.row()); 
            switch (static_cast<Column>(index.column())) {
            case IdentifierColumn: return subscriber.subscriber();
            case IsAllMonitoredColumn: return subscriber.isAllMonitored();
            case MonitoredCollectionsColumn: return toString(subscriber.monitoredCollections());
            case MonitoredItemsColumn: return toString(subscriber.monitoredItems());
            case MonitoredTagsColumn: return toString(subscriber.monitoredTags());
            case MonitoredResourcesColumn: return toString(subscriber.monitoredResources());
            case MonitoredMimeTypesColumn: return toString(subscriber.monitoredMimeTypes());
            case MonitoredTypesColumn: return toString(subscriber.monitoredTypes());
            case IgnoredSessionsColumn: return toString(subscriber.ignoredSessions());
            case IsExclusiveColumn: return subscriber.isExclusive();
            case ColumnsCount: Q_ASSERT(false); return QString();
            }
        }
    }

    return QVariant();
}

int MonitorsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnsCount;
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

    return QModelIndex();
}

QModelIndex MonitorsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, -1);
    }

    return createIndex(row, column, parent.row());
}

