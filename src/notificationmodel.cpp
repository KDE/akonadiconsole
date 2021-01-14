/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "notificationmodel.h"

#include <QLocale>

#include "akonadiconsole_debug.h"
#include <AkonadiCore/ServerManager>

#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/protocol_p.h>
#include <QMetaMethod>

Q_DECLARE_METATYPE(Akonadi::ChangeNotification)

using namespace Akonadi;

NotificationModel::NotificationModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

NotificationModel::~NotificationModel()
{
    setEnabled(false);
}

int NotificationModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _ColumnCount;
}

int NotificationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.count();
}

QModelIndex NotificationModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || row >= m_data.count() || column < 0 || column > 9 || parent.isValid()) {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex NotificationModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return {};
}

QVariant NotificationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        const auto msg = m_data.at(index.row());
        switch (index.column()) {
        case DateColumn:
            return msg.timestamp().toString(Qt::ISODateWithMs);
        case TypeColumn:
            switch (msg.type()) {
            case ChangeNotification::Items:
                return QStringLiteral("Items");
            case ChangeNotification::Collection:
                return QStringLiteral("Collection");
            case ChangeNotification::Tag:
                return QStringLiteral("Tag");
            case ChangeNotification::Relation:
                return QStringLiteral("Relation");
            case ChangeNotification::Subscription:
                return QStringLiteral("Subscription");
            }
            return QStringLiteral("Unknown");
        case OperationColumn:
            switch (msg.type()) {
            case ChangeNotification::Items:
                switch (Protocol::cmdCast<Protocol::ItemChangeNotification>(msg.notification()).operation()) {
                case Protocol::ItemChangeNotification::Add:
                    return QStringLiteral("Add");
                case Protocol::ItemChangeNotification::Modify:
                    return QStringLiteral("Modify");
                case Protocol::ItemChangeNotification::Move:
                    return QStringLiteral("Move");
                case Protocol::ItemChangeNotification::Remove:
                    return QStringLiteral("Remove");
                case Protocol::ItemChangeNotification::Link:
                    return QStringLiteral("Link");
                case Protocol::ItemChangeNotification::Unlink:
                    return QStringLiteral("Unlink");
                case Protocol::ItemChangeNotification::ModifyFlags:
                    return QStringLiteral("ModifyFlags");
                case Protocol::ItemChangeNotification::ModifyTags:
                    return QStringLiteral("ModifyTags");
                case Protocol::ItemChangeNotification::ModifyRelations:
                    return QStringLiteral("ModifyRelations");
                case Protocol::ItemChangeNotification::InvalidOp:
                    return QStringLiteral("InvalidOp");
                }
                return {};
            case ChangeNotification::Collection:
                switch (Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg.notification()).operation()) {
                case Protocol::CollectionChangeNotification::Add:
                    return QStringLiteral("Add");
                case Protocol::CollectionChangeNotification::Modify:
                    return QStringLiteral("Modify");
                case Protocol::CollectionChangeNotification::Move:
                    return QStringLiteral("Move");
                case Protocol::CollectionChangeNotification::Remove:
                    return QStringLiteral("Remove");
                case Protocol::CollectionChangeNotification::Subscribe:
                    return QStringLiteral("Subscribe");
                case Protocol::CollectionChangeNotification::Unsubscribe:
                    return QStringLiteral("Unsubscribe");
                case Protocol::CollectionChangeNotification::InvalidOp:
                    return QStringLiteral("InvalidIp");
                }
                return {};
            case ChangeNotification::Tag:
                switch (Protocol::cmdCast<Protocol::TagChangeNotification>(msg.notification()).operation()) {
                case Protocol::TagChangeNotification::Add:
                    return QStringLiteral("Add");
                case Protocol::TagChangeNotification::Modify:
                    return QStringLiteral("Modify");
                case Protocol::TagChangeNotification::Remove:
                    return QStringLiteral("Remove");
                case Protocol::TagChangeNotification::InvalidOp:
                    return QStringLiteral("InvalidOp");
                }
                return {};
            case ChangeNotification::Relation:
                switch (Protocol::cmdCast<Protocol::RelationChangeNotification>(msg.notification()).operation()) {
                case Protocol::RelationChangeNotification::Add:
                    return QStringLiteral("Add");
                case Protocol::RelationChangeNotification::Remove:
                    return QStringLiteral("Remove");
                case Protocol::RelationChangeNotification::InvalidOp:
                    return QStringLiteral("InvalidOp");
                }
                return {};
            case ChangeNotification::Subscription:
                switch (Protocol::cmdCast<Protocol::SubscriptionChangeNotification>(msg.notification()).operation()) {
                case Akonadi::Protocol::SubscriptionChangeNotification::Add:
                    return QStringLiteral("Add");
                case Akonadi::Protocol::SubscriptionChangeNotification::Modify:
                    return QStringLiteral("Modify");
                case Akonadi::Protocol::SubscriptionChangeNotification::Remove:
                    return QStringLiteral("Remove");
                case Akonadi::Protocol::SubscriptionChangeNotification::InvalidOp:
                    return QStringLiteral("InvalidOp");
                }
                return {};
            default:
                return QStringLiteral("Unknown");
            }
        case IdsColumn:
            switch (msg.type()) {
            case ChangeNotification::Items:
            {
                QStringList rv;
                const auto items = Protocol::cmdCast<Protocol::ItemChangeNotification>(msg.notification()).items();
                for (const auto &item : items) {
                    rv.push_back(QString::number(item.id()));
                }
                return rv.join(QLatin1String(", "));
            }
            case ChangeNotification::Collection:
                return Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg.notification()).collection().id();
            case ChangeNotification::Tag:
                return Protocol::cmdCast<Protocol::TagChangeNotification>(msg.notification()).tag().id();
            case ChangeNotification::Relation:
            case ChangeNotification::Subscription:
                return {};
            }
            return {};
        case SessionColumn:
            return msg.notification()->sessionId();
        case ListenersColumn:
        {
            const auto listeners = msg.listeners();
            QStringList rv;
            for (const auto &l : listeners) {
                rv.push_back(QString::fromUtf8(l));
            }
            return rv.join(QLatin1String(", "));
        }
        }
    } else if (role == NotificationRole) {
        return QVariant::fromValue(m_data.at(index.row()));
    }

    return QVariant();
}

QVariant NotificationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case DateColumn:
            return QStringLiteral("Date");
        case TypeColumn:
            return QStringLiteral("Type");
        case OperationColumn:
            return QStringLiteral("Operation");
        case IdsColumn:
            return QStringLiteral("IDs");
        case SessionColumn:
            return QStringLiteral("Session");
        case ListenersColumn:
            return QStringLiteral("Listeners");
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

void NotificationModel::slotNotify(const Akonadi::ChangeNotification &ntf)
{
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.append(ntf);
    endInsertRows();
}

void NotificationModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

void NotificationModel::setEnabled(bool enable)
{
    if (enable) {
        m_monitor = new Akonadi::Monitor(this);
        m_monitor->setObjectName(QStringLiteral("notificationMonitor"));
        m_monitor->setTypeMonitored(Akonadi::Monitor::Notifications);
        connect(m_monitor, &Akonadi::Monitor::debugNotification,
                this, &NotificationModel::slotNotify);
    } else if (m_monitor) {
        m_monitor->deleteLater();
        m_monitor = nullptr;
    }
}
