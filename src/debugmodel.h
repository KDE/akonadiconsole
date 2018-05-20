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

#ifndef AKONADICONSOLE_DEBUGMODEL_H_
#define AKONADICONSOLE_DEBUGMODEL_H_

#include <QAbstractItemModel>
#include <QMap>

class QStandardItemModel;

class DebugModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Direction {
        ClientToServer,
        ServerToClient
    };

    struct Message {
        QString sender;
        Direction direction;
        QString message;
    };

    enum Roles {
        MessageRole = Qt::UserRole,
        IdentifierRole
    };

    enum Columns {
        DirectionColumn,
        SenderColumn,
        MessageColumn,

        _ColumnCount
    };


    explicit DebugModel(QObject *parent = nullptr);
    ~DebugModel() override;

    void addMessage(const QString &sender, Direction direction, const QString &message);

    void setSenderFilterModel(QStandardItemModel *senderFilterModel);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child = {}) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    QString cacheString(const QString &str, QMap<QString, QString> &cache, QStandardItemModel *model = nullptr);
    QString displaySender(const QString &identifer) const;

    QVector<Message> mMessages;
    QStandardItemModel *mSenderFilterModel = nullptr;
    QMap<QString,QString> mSenderCache;
};

#endif
