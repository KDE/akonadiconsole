/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractItemModel>

#include <AkonadiCore/ChangeNotification>
#include <AkonadiCore/Monitor>

namespace Akonadi
{
namespace Protocol
{
class ChangeNotification;
}
}

class NotificationModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Role { NotificationRole = Qt::UserRole };
    enum Columns {
        DateColumn,
        TypeColumn,
        OperationColumn,
        IdsColumn,
        SessionColumn,
        ListenersColumn,

        _ColumnCount
    };

    explicit NotificationModel(QObject *parent);
    ~NotificationModel();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Akonadi::Protocol::ChangeNotificationPtr notification(const QModelIndex &index) const;

    Q_REQUIRED_RESULT bool isEnabled() const
    {
        return m_monitor;
    }

public Q_SLOTS:
    void clear();
    void setEnabled(bool enable);

private Q_SLOTS:
    void slotNotify(const Akonadi::ChangeNotification &msg);

private:
    QList<Akonadi::ChangeNotification> m_data;

    Akonadi::Monitor *m_monitor = nullptr;
};

