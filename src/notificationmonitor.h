/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
namespace KPIM
{
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
