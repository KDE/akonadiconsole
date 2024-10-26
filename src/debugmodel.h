/*
    SPDX-FileCopyrightText: 2018 Sandro Knauß <sknauss@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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

    QList<Message> mMessages;
    QStandardItemModel *mSenderFilterModel = nullptr;
    QMap<QString, QString> mSenderCache;
};
