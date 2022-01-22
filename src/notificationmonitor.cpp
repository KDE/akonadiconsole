/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "notificationmonitor.h"
#include "notificationfiltermodel.h"
#include "notificationmodel.h"
#include "utils.h"

#include <Akonadi/ControlGui>

#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KSharedConfig>
#include <Libkdepim/KCheckComboBox>

using KPIM::KCheckComboBox;

#ifndef COMPILE_WITH_UNITY_CMAKE_SUPPORT
Q_DECLARE_METATYPE(Akonadi::ChangeNotification)
#endif

NotificationMonitor::NotificationMonitor(QWidget *parent)
    : QWidget(parent)
{
    m_model = new NotificationModel(this);
    m_model->setEnabled(false); // since it can be slow, default to off

    m_filterModel = new NotificationFilterModel(this);
    m_filterModel->setSourceModel(m_model);

    auto layout = new QVBoxLayout(this);
    auto hLayout = new QHBoxLayout;
    layout->addLayout(hLayout);

    auto enableCB = new QCheckBox(this);
    enableCB->setText(QStringLiteral("Enable notification monitor"));
    enableCB->setChecked(m_model->isEnabled());
    connect(enableCB, &QCheckBox::toggled, m_model, &NotificationModel::setEnabled);
    hLayout->addWidget(enableCB);

    hLayout->addWidget(new QLabel(QStringLiteral("Types:"), this));
    hLayout->addWidget(mTypeFilterCombo = new KCheckComboBox(this));
    hLayout->addStretch();
    mTypeFilterCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    mTypeFilterCombo->setMinimumWidth(fontMetrics().boundingRect(QStringLiteral("Subscription,Items,Collections")).width()
                                      + 60); // make it wide enough for most use cases
    m_filterModel->setTypeFilter(mTypeFilterCombo);

    m_splitter = new QSplitter(this);
    layout->addWidget(m_splitter);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_filterModel);
    m_treeView->expandAll();
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &NotificationMonitor::contextMenu);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &NotificationMonitor::onNotificationSelected);
    m_splitter->addWidget(m_treeView);

    m_ntfView = new QTreeView(this);
    m_ntfView->setModel(new QStandardItemModel(this));
    m_ntfView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_splitter->addWidget(m_ntfView);

    auto h = new QHBoxLayout;
    layout->addLayout(h);

    auto btn = new QPushButton(QStringLiteral("Save to File..."));
    connect(btn, &QPushButton::clicked, this, &NotificationMonitor::saveToFile);
    h->addWidget(btn);
    h->addStretch(1);

    onNotificationSelected({});

    KConfigGroup config(KSharedConfig::openConfig(), "NotificationMonitor");
    m_treeView->header()->restoreState(config.readEntry<QByteArray>("tv", QByteArray()));
    m_ntfView->header()->restoreState(config.readEntry<QByteArray>("ntfView", QByteArray()));
    m_splitter->restoreState(config.readEntry<QByteArray>("splitter", QByteArray()));

    Akonadi::ControlGui::widgetNeedsAkonadi(this);
}

NotificationMonitor::~NotificationMonitor()
{
    KConfigGroup config(KSharedConfig::openConfig(), "NotificationMonitor");
    config.writeEntry("tv", m_treeView->header()->saveState());
    config.writeEntry("ntfView", m_ntfView->header()->saveState());
    config.writeEntry("splitter", m_splitter->saveState());
}

void NotificationMonitor::contextMenu(const QPoint & /*pos*/)
{
    QMenu menu;
    menu.addAction(QStringLiteral("Clear View"), m_model, &NotificationModel::clear);
    menu.exec(QCursor::pos());
}

void NotificationMonitor::onNotificationSelected(const QModelIndex &index)
{
    const auto state = m_ntfView->header()->saveState();
    populateNtfModel(index);
    m_ntfView->header()->restoreState(state);
    m_ntfView->expandAll();
}

void NotificationMonitor::populateNtfModel(const QModelIndex &index)
{
    auto model = qobject_cast<QStandardItemModel *>(m_ntfView->model());
    model->clear();
    model->setHorizontalHeaderLabels({QStringLiteral("Properties"), QStringLiteral("Values")});

    const auto ntf = index.data(NotificationModel::NotificationRole).value<Akonadi::ChangeNotification>();
    if (!ntf.isValid()) {
        return;
    }

    appendRow(model, QStringLiteral("Timestamp"), ntf.timestamp().toString(Qt::ISODateWithMs));
    appendRow(model, QStringLiteral("Type"), index.sibling(index.row(), NotificationModel::TypeColumn).data().toString());
    appendRow(model, QStringLiteral("Listeners"), index.sibling(index.row(), NotificationModel::ListenersColumn).data().toString());
    switch (ntf.type()) {
    case Akonadi::ChangeNotification::Items:
        populateItemNtfTree(model, Akonadi::Protocol::cmdCast<Akonadi::Protocol::ItemChangeNotification>(ntf.notification()));
        break;
    case Akonadi::ChangeNotification::Collection:
        populateCollectionNtfTree(model, Akonadi::Protocol::cmdCast<Akonadi::Protocol::CollectionChangeNotification>(ntf.notification()));
        break;
    case Akonadi::ChangeNotification::Tag:
        populateTagNtfTree(model, Akonadi::Protocol::cmdCast<Akonadi::Protocol::TagChangeNotification>(ntf.notification()));
        break;
    case Akonadi::ChangeNotification::Relation:
        populateRelationNtfTree(model, Akonadi::Protocol::cmdCast<Akonadi::Protocol::RelationChangeNotification>(ntf.notification()));
        break;
    case Akonadi::ChangeNotification::Subscription:
        populateSubscriptionNtfTree(model, Akonadi::Protocol::cmdCast<Akonadi::Protocol::SubscriptionChangeNotification>(ntf.notification()));
        break;
    }
}

void NotificationMonitor::populateItemNtfTree(QStandardItemModel *model, const Akonadi::Protocol::ItemChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::ItemChangeNotification::Add:
        operation = QStringLiteral("Add");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Modify:
        operation = QStringLiteral("Modify");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Move:
        operation = QStringLiteral("Move");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Remove:
        operation = QStringLiteral("Remove");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Link:
        operation = QStringLiteral("Link");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Unlink:
        operation = QStringLiteral("Unlink");
        break;
    case Akonadi::Protocol::ItemChangeNotification::ModifyFlags:
        operation = QStringLiteral("ModifyFlags");
        break;
    case Akonadi::Protocol::ItemChangeNotification::ModifyTags:
        operation = QStringLiteral("ModifyTags");
        break;
    case Akonadi::Protocol::ItemChangeNotification::ModifyRelations:
        operation = QStringLiteral("ModifyRelations");
        break;
    case Akonadi::Protocol::ItemChangeNotification::InvalidOp:
        operation = QStringLiteral("InvalidOp");
        break;
    }

    appendRow(model, QStringLiteral("Operation"), operation);
    appendRow(model, QStringLiteral("Resource"), QString::fromUtf8(ntf.resource()));
    appendRow(model, QStringLiteral("Parent Collection"), QString::number(ntf.parentCollection()));
    appendRow(model, QStringLiteral("Parent Dest Col"), QString::number(ntf.parentDestCollection()));
    appendRow(model, QStringLiteral("Destination Resource"), QString::fromUtf8(ntf.destinationResource()));
    appendRow(model, QStringLiteral("Item Parts"), toString(ntf.itemParts()));
    appendRow(model, QStringLiteral("Added Flags"), toString(ntf.addedFlags()));
    appendRow(model, QStringLiteral("Removed Flags"), toString(ntf.removedFlags()));
    appendRow(model, QStringLiteral("Added Tags"), toString(ntf.addedTags()));
    appendRow(model, QStringLiteral("Removed Tags"), toString(ntf.removedTags()));
    auto relationsItem = new QStandardItem(QStringLiteral("Added Relations"));
    const auto addedRelations = ntf.addedRelations();
    for (const auto &addedRelation : addedRelations) {
        auto item = new QStandardItem(
            QStringLiteral("%lld-%lld %s").arg(QString::number(addedRelation.leftId), QString::number(addedRelation.rightId), addedRelation.type));
        relationsItem->appendRow(item);
    }
    model->appendRow(relationsItem);

    relationsItem = new QStandardItem(QStringLiteral("Removed Relations"));
    const auto removedRelations = ntf.removedRelations();
    for (const auto &removedRelation : removedRelations) {
        auto item = new QStandardItem(
            QStringLiteral("%lld-%lld %s").arg(QString::number(removedRelation.leftId), QString::number(removedRelation.rightId), removedRelation.type));
        relationsItem->appendRow(item);
    }
    model->appendRow(relationsItem);

    appendRow(model, QStringLiteral("Must retrieve"), toString(ntf.mustRetrieve()));

    auto itemsItem = new QStandardItem(QStringLiteral("Items"));
    const auto &items = ntf.items();
    for (const auto &item : items) {
        auto i = new QStandardItem(QString::number(item.id()));
        populateItemTree(i, item);
        itemsItem->appendRow(i);
    }
    model->appendRow(itemsItem);
}

QStandardItem *NotificationMonitor::populateAncestorTree(QStandardItem *parent, const Akonadi::Protocol::Ancestor &ancestor)
{
    appendRow(parent, QStringLiteral("id"), QString::number(ancestor.id()));
    appendRow(parent, QStringLiteral("remoteId"), ancestor.remoteId());
    appendRow(parent, QStringLiteral("name"), ancestor.name());
    populateAttributesTree(parent, ancestor.attributes());
    auto ancestorItem = new QStandardItem(QStringLiteral("Ancestor"));
    parent->appendRow(ancestorItem);
    return ancestorItem;
}

void NotificationMonitor::populateTagTree(QStandardItem *parent, const Akonadi::Protocol::FetchTagsResponse &tag)
{
    appendRow(parent, QStringLiteral("ID"), QString::number(tag.id()));
    appendRow(parent, QStringLiteral("Parent ID"), QString::number(tag.parentId()));
    appendRow(parent, QStringLiteral("GID"), QString::fromUtf8(tag.gid()));
    appendRow(parent, QStringLiteral("Type"), QString::fromUtf8(tag.type()));
    appendRow(parent, QStringLiteral("Remote ID"), QString::fromUtf8(tag.remoteId()));
    populateAttributesTree(parent, tag.attributes());
}

void NotificationMonitor::populateAttributesTree(QStandardItem *parent, const Akonadi::Protocol::Attributes &attributes)
{
    auto attributesItem = new QStandardItem(QStringLiteral("Attributes"));
    for (auto it = attributes.cbegin(), end = attributes.cend(); it != end; ++it) {
        appendRow(attributesItem, QString::fromUtf8(it.key()), QString::fromUtf8(it.value()));
    }
    parent->appendRow(attributesItem);
}

void NotificationMonitor::populateItemTree(QStandardItem *parent, const Akonadi::Protocol::FetchItemsResponse &item)
{
    appendRow(parent, QStringLiteral("Revision"), QString::number(item.revision()));
    appendRow(parent, QStringLiteral("ParentID"), QString::number(item.parentId()));
    appendRow(parent, QStringLiteral("RemoteID"), item.remoteId());
    appendRow(parent, QStringLiteral("RemoteRev"), item.remoteRevision());
    appendRow(parent, QStringLiteral("GID"), item.gid());
    appendRow(parent, QStringLiteral("Size"), QString::number(item.size()));
    appendRow(parent, QStringLiteral("MimeType"), item.mimeType());
    appendRow(parent, QStringLiteral("MTime"), item.mTime().toString(Qt::ISODate));
    appendRow(parent, QStringLiteral("Flags"), toString(item.flags()));
    auto tagItem = new QStandardItem(QStringLiteral("Tags"));
    const auto tags = item.tags();
    for (const auto &tag : tags) {
        auto item = new QStandardItem(QString::number(tag.id()));
        populateTagTree(item, tag);
        tagItem->appendRow(item);
    }
    parent->appendRow(tagItem);

    appendRow(parent, QStringLiteral("VRefs"), toString(item.virtualReferences()));
    auto relationItem = new QStandardItem(QStringLiteral("Relations"));
    const auto relations = item.relations();
    for (const auto &relation : relations) {
        auto item = new QStandardItem(
            QStringLiteral("%lld-%lld %s").arg(QString::number(relation.left()), QString::number(relation.right()), QString::fromUtf8(relation.type())));
        relationItem->appendRow(item);
    }
    parent->appendRow(relationItem);

    const auto ancestors = item.ancestors();
    auto i = new QStandardItem(QStringLiteral("Ancestor"));
    parent->appendRow(i);
    for (const auto &ancestor : ancestors) {
        i = populateAncestorTree(i, ancestor);
    }

    auto partsItem = new QStandardItem(QStringLiteral("Parts"));
    const auto parts = item.parts();
    for (const auto &part : parts) {
        auto item = new QStandardItem(QString::fromUtf8(part.payloadName()));
        QString type;
        switch (part.metaData().storageType()) {
        case Akonadi::Protocol::PartMetaData::External:
            type = QStringLiteral("External");
            break;
        case Akonadi::Protocol::PartMetaData::Internal:
            type = QStringLiteral("Internal");
            break;
        case Akonadi::Protocol::PartMetaData::Foreign:
            type = QStringLiteral("Foreign");
            break;
        }
        appendRow(item, QStringLiteral("Size"), QString::number(part.metaData().size()));
        appendRow(item, QStringLiteral("Storage Type"), type);
        appendRow(item, QStringLiteral("Version"), QString::number(part.metaData().version()));
        appendRow(item, QStringLiteral("Data"), QString::fromUtf8(part.data().toHex()));

        partsItem->appendRow(item);
    }
    parent->appendRow(partsItem);
}

void NotificationMonitor::populateCollectionTree(QStandardItem *parent, const Akonadi::Protocol::FetchCollectionsResponse &collection)
{
    appendRow(parent, QStringLiteral("ID"), QString::number(collection.id()));
    appendRow(parent, QStringLiteral("Parent ID"), QString::number(collection.parentId()));
    appendRow(parent, QStringLiteral("Name"), collection.name());
    appendRow(parent, QStringLiteral("Mime Types"), toString(static_cast<QList<QString>>(collection.mimeTypes())));
    appendRow(parent, QStringLiteral("Remote ID"), collection.remoteId());
    appendRow(parent, QStringLiteral("Remote Revision"), collection.remoteRevision());
    auto statsItem = new QStandardItem(QStringLiteral("Statistics"));
    const auto stats = collection.statistics();
    appendRow(statsItem, QStringLiteral("Count"), QString::number(stats.count()));
    appendRow(statsItem, QStringLiteral("Unseen"), QString::number(stats.unseen()));
    appendRow(statsItem, QStringLiteral("Size"), QString::number(stats.size()));
    parent->appendRow(statsItem);
    appendRow(parent, QStringLiteral("Search Query"), collection.searchQuery());
    appendRow(parent, QStringLiteral("Search Collections"), toString(collection.searchCollections()));
    auto i = new QStandardItem(QStringLiteral("Ancestor"));
    parent->appendRow(i);
    const auto ancestors = collection.ancestors();
    for (const auto &ancestor : ancestors) {
        i = populateAncestorTree(i, ancestor);
    }
    auto cpItem = new QStandardItem(QStringLiteral("Cache Policy"));
    const auto cp = collection.cachePolicy();
    appendRow(cpItem, QStringLiteral("Inherit"), toString(cp.inherit()));
    appendRow(cpItem, QStringLiteral("Check Interval"), QString::number(cp.checkInterval()));
    appendRow(cpItem, QStringLiteral("Cache Timeout"), QString::number(cp.cacheTimeout()));
    appendRow(cpItem, QStringLiteral("Sync on Demand"), toString(cp.syncOnDemand()));
    appendRow(cpItem, QStringLiteral("Local Parts"), toString(static_cast<QList<QString>>(cp.localParts())));
    parent->appendRow(cpItem);

    populateAttributesTree(parent, collection.attributes());

    appendRow(parent, QStringLiteral("Enabled"), toString(collection.enabled()));
    appendRow(parent, QStringLiteral("DisplayPref"), toString(collection.displayPref()));
    appendRow(parent, QStringLiteral("SyncPref"), toString(collection.syncPref()));
    appendRow(parent, QStringLiteral("IndexPref"), toString(collection.indexPref()));
    appendRow(parent, QStringLiteral("Virtual"), toString(collection.isVirtual()));
}

void NotificationMonitor::populateCollectionNtfTree(QStandardItemModel *model, const Akonadi::Protocol::CollectionChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::CollectionChangeNotification::Add:
        operation = QStringLiteral("Add");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Modify:
        operation = QStringLiteral("Modify");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Move:
        operation = QStringLiteral("Move");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Remove:
        operation = QStringLiteral("Remove");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Subscribe:
        operation = QStringLiteral("Subscribe");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Unsubscribe:
        operation = QStringLiteral("Unsubscribe");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::InvalidOp:
        operation = QStringLiteral("InvalidIp");
        break;
    }
    appendRow(model, QStringLiteral("Operation"), operation);
    appendRow(model, QStringLiteral("Resource"), QString::fromUtf8(ntf.resource()));
    appendRow(model, QStringLiteral("Parent Collection"), QString::number(ntf.parentCollection()));
    appendRow(model, QStringLiteral("Parent Dest Collection"), QString::number(ntf.parentDestCollection()));
    appendRow(model, QStringLiteral("Destination Resource"), QString::fromUtf8(ntf.destinationResource()));
    appendRow(model, QStringLiteral("Changed Parts"), toString(ntf.changedParts()));
    auto item = new QStandardItem(QStringLiteral("Collection"));
    populateCollectionTree(item, ntf.collection());
    model->appendRow(item);
}

void NotificationMonitor::populateRelationNtfTree(QStandardItemModel *model, const Akonadi::Protocol::RelationChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::RelationChangeNotification::Add:
        operation = QStringLiteral("Add");
        break;
    case Akonadi::Protocol::RelationChangeNotification::Remove:
        operation = QStringLiteral("Remove");
        break;
    case Akonadi::Protocol::RelationChangeNotification::InvalidOp:
        operation = QStringLiteral("InvalidOp");
        break;
    }
    appendRow(model, QStringLiteral("Operation"), operation);
    auto item = new QStandardItem(QStringLiteral("Relation"));
    const auto rel = ntf.relation();
    appendRow(item, QStringLiteral("Left ID"), QString::number(rel.left()));
    appendRow(item, QStringLiteral("Left MimeType"), QString::fromUtf8(rel.leftMimeType()));
    appendRow(item, QStringLiteral("Right ID"), QString::number(rel.right()));
    appendRow(item, QStringLiteral("Right MimeType"), QString::fromUtf8(rel.rightMimeType()));
    appendRow(item, QStringLiteral("Remote ID"), QString::fromUtf8(rel.remoteId()));
    model->appendRow(item);
}

void NotificationMonitor::populateTagNtfTree(QStandardItemModel *model, const Akonadi::Protocol::TagChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::TagChangeNotification::Add:
        operation = QStringLiteral("Add");
        break;
    case Akonadi::Protocol::TagChangeNotification::Modify:
        operation = QStringLiteral("Modify");
        break;
    case Akonadi::Protocol::TagChangeNotification::Remove:
        operation = QStringLiteral("Remove");
        break;
    case Akonadi::Protocol::TagChangeNotification::InvalidOp:
        operation = QStringLiteral("InvalidOp");
        break;
    }
    appendRow(model, QStringLiteral("Operation"), operation);
    appendRow(model, QStringLiteral("Resource"), QString::fromUtf8(ntf.resource()));
    auto tagItem = new QStandardItem(QStringLiteral("Tag"));
    populateTagTree(tagItem, ntf.tag());
    model->appendRow(tagItem);
}

void NotificationMonitor::populateSubscriptionNtfTree(QStandardItemModel *model, const Akonadi::Protocol::SubscriptionChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::SubscriptionChangeNotification::Add:
        operation = QStringLiteral("Add");
        break;
    case Akonadi::Protocol::SubscriptionChangeNotification::Modify:
        operation = QStringLiteral("Modify");
        break;
    case Akonadi::Protocol::SubscriptionChangeNotification::Remove:
        operation = QStringLiteral("Remove");
        break;
    case Akonadi::Protocol::SubscriptionChangeNotification::InvalidOp:
        operation = QStringLiteral("InvalidOp");
        break;
    }
    appendRow(model, QStringLiteral("Operation"), operation);
    appendRow(model, QStringLiteral("Subscriber"), QString::fromUtf8(ntf.subscriber()));
    appendRow(model, QStringLiteral("Monitored Collections"), toString(ntf.collections()));
    appendRow(model, QStringLiteral("Monitored Items"), toString(ntf.items()));
    appendRow(model, QStringLiteral("Monitored Tags"), toString(ntf.tags()));
    QStringList types;
    const auto typesSet = ntf.types();
    for (const auto &type : typesSet) {
        switch (type) {
        case Akonadi::Protocol::ModifySubscriptionCommand::ItemChanges:
            types.push_back(QStringLiteral("Items"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::CollectionChanges:
            types.push_back(QStringLiteral("Collections"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::TagChanges:
            types.push_back(QStringLiteral("Tags"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::RelationChanges:
            types.push_back(QStringLiteral("Relations"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::SubscriptionChanges:
            types.push_back(QStringLiteral("Subscriptions"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::ChangeNotifications:
            types.push_back(QStringLiteral("Changes"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::NoType:
            types.push_back(QStringLiteral("No Type"));
            break;
        }
    }
    appendRow(model, QStringLiteral("Monitored Types"), types.join(QLatin1String(", ")));
    appendRow(model, QStringLiteral("Monitored Mime Types"), toString(ntf.mimeTypes()));
    appendRow(model, QStringLiteral("Monitored Resources"), toString(ntf.resources()));
    appendRow(model, QStringLiteral("Ignored Sessions"), toString(ntf.ignoredSessions()));
    appendRow(model, QStringLiteral("All Monitored"), toString(ntf.allMonitored()));
    appendRow(model, QStringLiteral("Exclusive"), toString(ntf.exclusive()));

    auto item = new QStandardItem(QStringLiteral("Item Fetch Scope"));
    const auto ifs = ntf.itemFetchScope();
    appendRow(item, QStringLiteral("Requested Parts"), toString(ifs.requestedParts()));
    appendRow(item, QStringLiteral("Changed Since"), ifs.changedSince().toString(Qt::ISODateWithMs));
    QString ancestorDepth;
    switch (ifs.ancestorDepth()) {
    case Akonadi::Protocol::ItemFetchScope::NoAncestor:
        ancestorDepth = QStringLiteral("No Ancestor");
        break;
    case Akonadi::Protocol::ItemFetchScope::ParentAncestor:
        ancestorDepth = QStringLiteral("Parent  Ancestor");
        break;
    case Akonadi::Protocol::ItemFetchScope::AllAncestors:
        ancestorDepth = QStringLiteral("All Ancestors");
        break;
    }
    appendRow(item, QStringLiteral("Ancestor Depth"), ancestorDepth);
    appendRow(item, QStringLiteral("Cache Only"), toString(ifs.cacheOnly()));
    appendRow(item, QStringLiteral("Check Cached Payload Parts Only"), toString(ifs.checkCachedPayloadPartsOnly()));
    appendRow(item, QStringLiteral("Full Payload"), toString(ifs.fullPayload()));
    appendRow(item, QStringLiteral("All Attributes"), toString(ifs.allAttributes()));
    appendRow(item, QStringLiteral("Fetch Size"), toString(ifs.fetchSize()));
    appendRow(item, QStringLiteral("Fetch MTime"), toString(ifs.fetchMTime()));
    appendRow(item, QStringLiteral("Fetch Remote Revision"), toString(ifs.fetchRemoteRevision()));
    appendRow(item, QStringLiteral("Ignore Errors"), toString(ifs.ignoreErrors()));
    appendRow(item, QStringLiteral("Fetch Flags"), toString(ifs.fetchFlags()));
    appendRow(item, QStringLiteral("Fetch RemoteID"), toString(ifs.fetchRemoteId()));
    appendRow(item, QStringLiteral("Fetch GID"), toString(ifs.fetchGID()));
    appendRow(item, QStringLiteral("Fetch Tags"), toString(ifs.fetchTags()));
    appendRow(item, QStringLiteral("Fetch Relations"), toString(ifs.fetchRelations()));
    appendRow(item, QStringLiteral("Fetch VRefs"), toString(ifs.fetchVirtualReferences()));
    model->appendRow(item);

    item = new QStandardItem(QStringLiteral("Collection Fetch Scope"));
    const auto cfs = ntf.collectionFetchScope();
    QString listFilter;
    switch (cfs.listFilter()) {
    case Akonadi::Protocol::CollectionFetchScope::NoFilter:
        listFilter = QStringLiteral("No Filter");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Display:
        listFilter = QStringLiteral("Display");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Enabled:
        listFilter = QStringLiteral("Enabled");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Index:
        listFilter = QStringLiteral("Index");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Sync:
        listFilter = QStringLiteral("Sync");
        break;
    }
    appendRow(item, QStringLiteral("List Filter"), listFilter);
    appendRow(item, QStringLiteral("Include Statistics"), toString(cfs.includeStatistics()));
    appendRow(item, QStringLiteral("Resource"), cfs.resource());
    appendRow(item, QStringLiteral("Content Mime Types"), cfs.contentMimeTypes().join(QLatin1String(", ")));
    appendRow(item, QStringLiteral("Attributes"), toString(cfs.attributes()));
    appendRow(item, QStringLiteral("Fetch ID Only"), toString(cfs.fetchIdOnly()));
    QString ancestorRetrieval;
    switch (cfs.ancestorRetrieval()) {
    case Akonadi::Protocol::CollectionFetchScope::All:
        ancestorRetrieval = QStringLiteral("All");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Parent:
        ancestorRetrieval = QStringLiteral("Parent");
        break;
    case Akonadi::Protocol::CollectionFetchScope::None:
        ancestorRetrieval = QStringLiteral("None");
        break;
    }
    appendRow(item, QStringLiteral("Ancestor Retrieval"), ancestorRetrieval);
    appendRow(item, QStringLiteral("Ancestor Fetch ID Only"), toString(cfs.ancestorFetchIdOnly()));
    appendRow(item, QStringLiteral("Ancestor Attributes"), toString(cfs.ancestorAttributes()));
    appendRow(item, QStringLiteral("Ignore Retrieval Errors"), toString(cfs.ignoreRetrievalErrors()));
    model->appendRow(item);

    item = new QStandardItem(QStringLiteral("Tag Fetch Scope"));
    const Akonadi::Protocol::TagFetchScope tfs = ntf.tagFetchScope();
    appendRow(item, QStringLiteral("Fetch ID Only"), toString(tfs.fetchIdOnly()));
    appendRow(item, QStringLiteral("Fetch RemoteID"), toString(tfs.fetchRemoteID()));
    appendRow(item, QStringLiteral("Fetch All Attributes"), toString(tfs.fetchAllAttributes()));
    appendRow(item, QStringLiteral("Attributes"), toString(tfs.attributes()));
    model->appendRow(item);
}

void NotificationMonitor::saveToFile()
{
    const auto filename = QFileDialog::getSaveFileName(this, QStringLiteral("Save to File..."));
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Failed to open file: %1").arg(file.errorString()));
        return;
    }

    QJsonObject json;
    auto ntfModel = qobject_cast<QStandardItemModel *>(m_ntfView->model());
    Q_ASSERT(ntfModel);

    QJsonArray rowArray;
    // Note that the use of m_model here means we save everything, not just what's visible (in case of filtering).
    const int rows = m_model->rowCount();
    for (int row = 0; row < rows; ++row) {
        QJsonObject rowObject;
        // The Ntf model has all the data that m_model has in its columns, apart from the session
        rowObject.insert(QStringLiteral("Session"), m_model->index(row, NotificationModel::SessionColumn).data().toString());

        populateNtfModel(m_model->index(row, 0));
        for (int r = 0, cnt = ntfModel->rowCount(); r < cnt; ++r) {
            const auto idx0 = ntfModel->index(r, 0);
            const auto idx1 = ntfModel->index(r, 1);
            rowObject.insert(idx0.data().toString(), QJsonValue::fromVariant(idx1.data()));
        }

        rowArray.append(rowObject);
    }
    json.insert(QStringLiteral("notifications"), rowArray);
    QJsonDocument saveDoc(json);
    file.write(saveDoc.toJson());
}
