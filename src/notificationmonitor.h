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

#ifndef AKONADICONSOLE_NOTIFICATIONMONITOR_H
#define AKONADICONSOLE_NOTIFICATIONMONITOR_H

#include <QWidget>
#include <akonadi/private/protocol_p.h>

class QModelIndex;
class NotificationModel;
class NotificationFilterModel;
class QTreeView;
class QStandardItem;
class QStandardItemModel;
class QSplitter;
namespace KPIM {
class KCheckComboBox;
}

class NotificationMonitor : public QWidget
{
    Q_OBJECT
public:
    explicit NotificationMonitor(QWidget *parent);
    ~NotificationMonitor() override;

private Q_SLOTS:
    void contextMenu(const QPoint &pos);

private:
    void onNotificationSelected(const QModelIndex &index);

    void populateItemNtfTree(QStandardItemModel *model, const Akonadi::Protocol::ItemChangeNotification &ntf);
    void populateCollectionNtfTree(QStandardItemModel *mddel, const Akonadi::Protocol::CollectionChangeNotification &ntf);
    void populateTagNtfTree(QStandardItemModel *model, const Akonadi::Protocol::TagChangeNotification &ntf);
    void populateRelationNtfTree(QStandardItemModel *model, const Akonadi::Protocol::RelationChangeNotification &ntf);
    void populateSubscriptionNtfTree(QStandardItemModel *model, const Akonadi::Protocol::SubscriptionChangeNotification &ntf);

    void populateItemTree(QStandardItem *parent, const Akonadi::Protocol::FetchItemsResponse &item);
    void populateCollectionTree(QStandardItem *parent, const Akonadi::Protocol::FetchCollectionsResponse &collection);
    void populateTagTree(QStandardItem *parent, const Akonadi::Protocol::FetchTagsResponse &tag);
    void populateAttributesTree(QStandardItem *parent, const Akonadi::Protocol::Attributes &attributes);
    QStandardItem *populateAncestorTree(QStandardItem *parent, const Akonadi::Protocol::Ancestor &ancestor);

    void populateNtfModel(const QModelIndex &index);
    void saveToFile();

    NotificationModel *m_model = nullptr;
    QSplitter *m_splitter = nullptr;
    QTreeView *m_treeView = nullptr;
    QTreeView *m_ntfView = nullptr;
    KPIM::KCheckComboBox *mTypeFilterCombo = nullptr;
    NotificationFilterModel *m_filterModel = nullptr;
};

#endif
