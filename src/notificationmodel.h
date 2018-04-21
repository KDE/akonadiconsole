/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef AKONADICONSOLE_NOTIFICATIONMODEL_H
#define AKONADICONSOLE_NOTIFICATIONMODEL_H

#include <QAbstractItemModel>

#include <AkonadiCore/Monitor>
#include <AkonadiCore/ChangeNotification>

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
    enum Role {
        NotificationRole = Qt::UserRole
    };
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

    bool isEnabled() const
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

#endif
