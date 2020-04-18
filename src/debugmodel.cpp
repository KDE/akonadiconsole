/*
    Copyright (c) 2018 Sandro Knauß <sknauss@kde.org>

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

#include "debugmodel.h"

#include <QDebug>
#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QStandardItemModel>

Q_DECLARE_METATYPE(DebugModel::Message)

DebugModel::DebugModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

DebugModel::~DebugModel()
{
}

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
        identifier = str.mid(pos+1,str.size()-pos-2);
        name = str.mid(0,pos-1);
    }
    if (! cache.contains(identifier)) {
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

void DebugModel::addMessage(const QString& sender, DebugModel::Direction direction, const QString& message)
{
    beginInsertRows({}, mMessages.count(), mMessages.count());
    mMessages.push_back({ cacheString(sender, mSenderCache, mSenderFilterModel), direction,
                          message });
    endInsertRows();
}

bool DebugModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid()) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    mMessages.remove(row, count);

    QVector<QString> toDelete;

    // find elements that needs to be deleted.
    for(const auto &identifer: mSenderCache.keys()) {
        bool found = false;
        for(const auto &msg: qAsConst(mMessages)) {
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
    for(const auto &i: toDelete) {

        const auto &item = mSenderFilterModel->findItems(displaySender(i));
        if (!item.isEmpty()) {
            const auto &index = item.first()->index();
            mSenderFilterModel->removeRows(index.row(),1);
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

int DebugModel::rowCount(const QModelIndex& parent) const
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
        return QStringLiteral("Sender");
    case DirectionColumn:
        return QStringLiteral("Direction");
    case MessageColumn:
        return QStringLiteral("Message");
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
