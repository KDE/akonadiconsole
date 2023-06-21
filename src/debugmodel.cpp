/*
    SPDX-FileCopyrightText: 2018 Sandro Knauß <sknauss@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "debugmodel.h"

#include <KLocalizedString>

#include <QColor>
#include <QStandardItemModel>

#ifndef COMPILE_WITH_UNITY_CMAKE_SUPPORT
Q_DECLARE_METATYPE(DebugModel::Message)
#endif

DebugModel::DebugModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

DebugModel::~DebugModel() = default;

QString DebugModel::displaySender(const QString &identifer) const
{
    if (mSenderCache[identifer].isEmpty()) {
        return identifer;
    } else {
        return QString(mSenderCache[identifer] + QStringLiteral(" (") + identifer + QStringLiteral(")"));
    }
}

QString DebugModel::cacheString(const QString &str, QMap<QString, QString> &cache, QStandardItemModel *model)
{
    auto pos = str.lastIndexOf(QLatin1Char('('));
    auto identifier = str;
    QString name;
    if (pos != -1) {
        identifier = str.mid(pos + 1, str.size() - pos - 2);
        name = str.mid(0, pos - 1);
    }
    if (!cache.contains(identifier)) {
        cache.insert(identifier, name);
        if (model) {
            auto item = new QStandardItem(displaySender(identifier));
            item->setData(identifier, IdentifierRole);
            item->setCheckState(Qt::Checked);
            model->appendRow(item);
        }
        return identifier;
    } else if (cache[identifier].isEmpty()) {
        cache[identifier] = name;
        const auto item = model->findItems(identifier).constFirst();
        item->setData(displaySender(identifier), Qt::DisplayRole);
    }
    return identifier;
}

void DebugModel::addMessage(const QString &sender, DebugModel::Direction direction, const QString &message)
{
    beginInsertRows({}, mMessages.count(), mMessages.count());
    mMessages.push_back({cacheString(sender, mSenderCache, mSenderFilterModel), direction, message});
    endInsertRows();
}

bool DebugModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid()) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    mMessages.remove(row, count);

    QList<QString> toDelete;

    // find elements that needs to be deleted.
    for (const auto &identifer : mSenderCache.keys()) {
        bool found = false;
        for (const auto &msg : std::as_const(mMessages)) {
            if (msg.sender == identifer) {
                found = true;
                break;
            }
        }
        if (!found) {
            toDelete.push_back(identifer);
        }
    }

    // Update senderCache and senderFilterModel
    for (const auto &i : toDelete) {
        const auto &item = mSenderFilterModel->findItems(displaySender(i));
        if (!item.isEmpty()) {
            const auto &index = item.first()->index();
            mSenderFilterModel->removeRows(index.row(), 1);
        }
        mSenderCache.remove(i);
    }
    endRemoveRows();
    return true;
}

void DebugModel::setSenderFilterModel(QStandardItemModel *senderFilterModel)
{
    mSenderFilterModel = senderFilterModel;
}

int DebugModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mMessages.count();
}

int DebugModel::columnCount(const QModelIndex &) const
{
    return _ColumnCount;
}

QModelIndex DebugModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= mMessages.count() || column < 0 || column >= _ColumnCount) {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex DebugModel::parent(const QModelIndex &) const
{
    return {};
}

QVariant DebugModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case SenderColumn:
        return i18n("Sender");
    case DirectionColumn:
        return i18n("Direction");
    case MessageColumn:
        return i18n("Message");
    }
    return {};
}

QVariant DebugModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mMessages.count() || index.column() >= _ColumnCount) {
        return {};
    }

    const auto message = mMessages.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case SenderColumn:
            return displaySender(message.sender);
        case DirectionColumn:
            switch (message.direction) {
            case ClientToServer:
                return QStringLiteral("<-");
            case ServerToClient:
                return QStringLiteral("->");
            }
            return {};
        case MessageColumn:
            return message.message;
        }
    } else if (role == Qt::ToolTipRole) {
        switch (index.column()) {
        case MessageColumn:
            return message.message;
        }
    } else if (role == Qt::ForegroundRole && index.column() != MessageColumn) {
        if (message.direction == ClientToServer) {
            return QColor(Qt::red);
        } else {
            return QColor(Qt::green);
        }
    } else if (role == MessageRole) {
        return QVariant::fromValue(message);
    } else if (role == IdentifierRole) {
        return message.sender;
    }

    return {};
}

#include "moc_debugmodel.cpp"
