/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "notificationmonitor.h"
using namespace Qt::Literals::StringLiterals;

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
#include <KLocalizedString>
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
    enableCB->setText(i18n("Enable notification monitor"));
    enableCB->setChecked(m_model->isEnabled());
    connect(enableCB, &QCheckBox::toggled, m_model, &NotificationModel::setEnabled);
    hLayout->addWidget(enableCB);

    hLayout->addWidget(new QLabel(i18nc("@label:textbox", "Types:"), this));
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

    auto btn = new QPushButton(i18nc("@action:button", "Save to File..."));
    connect(btn, &QPushButton::clicked, this, &NotificationMonitor::saveToFile);
    h->addWidget(btn);
    h->addStretch(1);

    onNotificationSelected({});

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("NotificationMonitor"));
    m_treeView->header()->restoreState(config.readEntry<QByteArray>("tv", QByteArray()));
    m_ntfView->header()->restoreState(config.readEntry<QByteArray>("ntfView", QByteArray()));
    m_splitter->restoreState(config.readEntry<QByteArray>("splitter", QByteArray()));

    Akonadi::ControlGui::widgetNeedsAkonadi(this);
}

NotificationMonitor::~NotificationMonitor()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("NotificationMonitor"));
    config.writeEntry("tv", m_treeView->header()->saveState());
    config.writeEntry("ntfView", m_ntfView->header()->saveState());
    config.writeEntry("splitter", m_splitter->saveState());
}

void NotificationMonitor::contextMenu(const QPoint & /*pos*/)
{
    QMenu menu;
    menu.addAction(i18n("Clear View"), m_model, &NotificationModel::clear);
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
    model->setHorizontalHeaderLabels({i18n("Properties"), i18n("Values")});

    const auto ntf = index.data(NotificationModel::NotificationRole).value<Akonadi::ChangeNotification>();
    if (!ntf.isValid()) {
        return;
    }

    appendRow(model, i18n("Timestamp"), ntf.timestamp().toString(Qt::ISODateWithMs));
    appendRow(model, i18n("Type"), index.sibling(index.row(), NotificationModel::TypeColumn).data().toString());
    appendRow(model, i18n("Listeners"), index.sibling(index.row(), NotificationModel::ListenersColumn).data().toString());
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
        operation = i18n("Add");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Modify:
        operation = i18n("Modify");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Move:
        operation = i18n("Move");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Remove:
        operation = i18n("Remove");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Link:
        operation = i18n("Link");
        break;
    case Akonadi::Protocol::ItemChangeNotification::Unlink:
        operation = i18n("Unlink");
        break;
    case Akonadi::Protocol::ItemChangeNotification::ModifyFlags:
        operation = i18n("ModifyFlags");
        break;
    case Akonadi::Protocol::ItemChangeNotification::ModifyTags:
        operation = i18n("ModifyTags");
        break;
    case Akonadi::Protocol::ItemChangeNotification::InvalidOp:
        operation = i18n("InvalidOp");
        break;
    }

    appendRow(model, i18n("Operation"), operation);
    appendRow(model, i18n("Resource"), QString::fromUtf8(ntf.resource()));
    appendRow(model, i18n("Parent Collection"), QString::number(ntf.parentCollection()));
    appendRow(model, i18n("Parent Dest Col"), QString::number(ntf.parentDestCollection()));
    appendRow(model, i18n("Destination Resource"), QString::fromUtf8(ntf.destinationResource()));
    appendRow(model, i18n("Item Parts"), toString(ntf.itemParts()));
    appendRow(model, i18n("Added Flags"), toString(ntf.addedFlags()));
    appendRow(model, i18n("Removed Flags"), toString(ntf.removedFlags()));

    {
        QSet<qint64> set;
        std::ranges::transform(ntf.addedTags(), std::inserter(set, set.begin()), [](const auto &tagResponse) -> Akonadi::Tag::Id {
            return tagResponse.id();
        });
        appendRow(model, i18n("Added Tags"), toString(set));
    }

    {
        QSet<qint64> set;
        std::ranges::transform(ntf.removedTags(), std::inserter(set, set.begin()), [](const auto &tagResponse) -> Akonadi::Tag::Id {
            return tagResponse.id();
        });
        appendRow(model, i18n("Removed Tags"), toString(set));
    }

    appendRow(model, i18n("Must retrieve"), toString(ntf.mustRetrieve()));

    auto itemsItem = new QStandardItem(i18n("Items"));
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
    appendRow(parent, i18n("id"), QString::number(ancestor.id()));
    appendRow(parent, i18n("remoteId"), ancestor.remoteId());
    appendRow(parent, i18n("name"), ancestor.name());
    populateAttributesTree(parent, ancestor.attributes());
    auto ancestorItem = new QStandardItem(i18n("Ancestor"));
    parent->appendRow(ancestorItem);
    return ancestorItem;
}

void NotificationMonitor::populateTagTree(QStandardItem *parent, const Akonadi::Protocol::FetchTagsResponse &tag)
{
    appendRow(parent, i18n("ID"), QString::number(tag.id()));
    appendRow(parent, i18n("Parent ID"), QString::number(tag.parentId()));
    appendRow(parent, i18n("GID"), QString::fromUtf8(tag.gid()));
    appendRow(parent, i18n("Type"), QString::fromUtf8(tag.type()));
    appendRow(parent, i18n("Remote ID"), QString::fromUtf8(tag.remoteId()));
    populateAttributesTree(parent, tag.attributes());
}

void NotificationMonitor::populateAttributesTree(QStandardItem *parent, const Akonadi::Protocol::Attributes &attributes)
{
    auto attributesItem = new QStandardItem(i18n("Attributes"));
    for (auto it = attributes.cbegin(), end = attributes.cend(); it != end; ++it) {
        appendRow(attributesItem, QString::fromUtf8(it.key()), QString::fromUtf8(it.value()));
    }
    parent->appendRow(attributesItem);
}

void NotificationMonitor::populateItemTree(QStandardItem *parent, const Akonadi::Protocol::FetchItemsResponse &item)
{
    appendRow(parent, i18n("Revision"), QString::number(item.revision()));
    appendRow(parent, i18n("ParentID"), QString::number(item.parentId()));
    appendRow(parent, i18n("RemoteID"), item.remoteId());
    appendRow(parent, i18n("RemoteRev"), item.remoteRevision());
    appendRow(parent, i18n("GID"), item.gid());
    appendRow(parent, i18n("Size"), QString::number(item.size()));
    appendRow(parent, i18n("MimeType"), item.mimeType());
    appendRow(parent, i18n("MTime"), item.mTime().toString(Qt::ISODate));
    appendRow(parent, i18n("Flags"), toString(item.flags()));
    auto tagItem = new QStandardItem(i18n("Tags"));
    const auto tags = item.tags();
    for (const auto &tag : tags) {
        auto item = new QStandardItem(QString::number(tag.id()));
        populateTagTree(item, tag);
        tagItem->appendRow(item);
    }
    parent->appendRow(tagItem);

    appendRow(parent, i18n("VRefs"), toString(item.virtualReferences()));

    const auto ancestors = item.ancestors();
    auto i = new QStandardItem(i18n("Ancestor"));
    parent->appendRow(i);
    for (const auto &ancestor : ancestors) {
        i = populateAncestorTree(i, ancestor);
    }

    auto partsItem = new QStandardItem(i18n("Parts"));
    const auto parts = item.parts();
    for (const auto &part : parts) {
        auto item = new QStandardItem(QString::fromUtf8(part.payloadName()));
        QString type;
        switch (part.metaData().storageType()) {
        case Akonadi::Protocol::PartMetaData::External:
            type = i18n("External");
            break;
        case Akonadi::Protocol::PartMetaData::Internal:
            type = i18n("Internal");
            break;
        case Akonadi::Protocol::PartMetaData::Foreign:
            type = i18n("Foreign");
            break;
        }
        appendRow(item, i18n("Size"), QString::number(part.metaData().size()));
        appendRow(item, i18n("Storage Type"), type);
        appendRow(item, i18n("Version"), QString::number(part.metaData().version()));
        appendRow(item, i18n("Data"), QString::fromUtf8(part.data().toHex()));

        partsItem->appendRow(item);
    }
    parent->appendRow(partsItem);
}

void NotificationMonitor::populateCollectionTree(QStandardItem *parent, const Akonadi::Protocol::FetchCollectionsResponse &collection)
{
    appendRow(parent, i18n("ID"), QString::number(collection.id()));
    appendRow(parent, i18n("Parent ID"), QString::number(collection.parentId()));
    appendRow(parent, i18n("Name"), collection.name());
    appendRow(parent, i18n("Mime Types"), toString(static_cast<QList<QString>>(collection.mimeTypes())));
    appendRow(parent, i18n("Remote ID"), collection.remoteId());
    appendRow(parent, i18n("Remote Revision"), collection.remoteRevision());
    auto statsItem = new QStandardItem(i18n("Statistics"));
    const auto stats = collection.statistics();
    appendRow(statsItem, i18n("Count"), QString::number(stats.count()));
    appendRow(statsItem, i18n("Unseen"), QString::number(stats.unseen()));
    appendRow(statsItem, i18n("Size"), QString::number(stats.size()));
    parent->appendRow(statsItem);
    appendRow(parent, i18n("Search Query"), collection.searchQuery());
    appendRow(parent, i18n("Search Collections"), toString(collection.searchCollections()));
    auto i = new QStandardItem(i18n("Ancestor"));
    parent->appendRow(i);
    const auto ancestors = collection.ancestors();
    for (const auto &ancestor : ancestors) {
        i = populateAncestorTree(i, ancestor);
    }
    auto cpItem = new QStandardItem(i18n("Cache Policy"));
    const auto cp = collection.cachePolicy();
    appendRow(cpItem, i18n("Inherit"), toString(cp.inherit()));
    appendRow(cpItem, i18n("Check Interval"), QString::number(cp.checkInterval()));
    appendRow(cpItem, i18n("Cache Timeout"), QString::number(cp.cacheTimeout()));
    appendRow(cpItem, i18n("Sync on Demand"), toString(cp.syncOnDemand()));
    appendRow(cpItem, i18n("Local Parts"), toString(static_cast<QList<QString>>(cp.localParts())));
    parent->appendRow(cpItem);

    populateAttributesTree(parent, collection.attributes());

    appendRow(parent, i18n("Enabled"), toString(collection.enabled()));
    appendRow(parent, i18n("DisplayPref"), toString(collection.displayPref()));
    appendRow(parent, i18n("SyncPref"), toString(collection.syncPref()));
    appendRow(parent, i18n("IndexPref"), toString(collection.indexPref()));
    appendRow(parent, i18n("Virtual"), toString(collection.isVirtual()));
}

void NotificationMonitor::populateCollectionNtfTree(QStandardItemModel *model, const Akonadi::Protocol::CollectionChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::CollectionChangeNotification::Add:
        operation = i18n("Add");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Modify:
        operation = i18n("Modify");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Move:
        operation = i18n("Move");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Remove:
        operation = i18n("Remove");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Subscribe:
        operation = i18n("Subscribe");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::Unsubscribe:
        operation = i18n("Unsubscribe");
        break;
    case Akonadi::Protocol::CollectionChangeNotification::InvalidOp:
        operation = i18n("InvalidIp");
        break;
    }
    appendRow(model, i18n("Operation"), operation);
    appendRow(model, i18n("Resource"), QString::fromUtf8(ntf.resource()));
    appendRow(model, i18n("Parent Collection"), QString::number(ntf.parentCollection()));
    appendRow(model, i18n("Parent Dest Collection"), QString::number(ntf.parentDestCollection()));
    appendRow(model, i18n("Destination Resource"), QString::fromUtf8(ntf.destinationResource()));
    appendRow(model, i18n("Changed Parts"), toString(ntf.changedParts()));
    auto item = new QStandardItem(i18n("Collection"));
    populateCollectionTree(item, ntf.collection());
    model->appendRow(item);
}

void NotificationMonitor::populateTagNtfTree(QStandardItemModel *model, const Akonadi::Protocol::TagChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::TagChangeNotification::Add:
        operation = i18n("Add");
        break;
    case Akonadi::Protocol::TagChangeNotification::Modify:
        operation = i18n("Modify");
        break;
    case Akonadi::Protocol::TagChangeNotification::Remove:
        operation = i18n("Remove");
        break;
    case Akonadi::Protocol::TagChangeNotification::InvalidOp:
        operation = i18n("InvalidOp");
        break;
    }
    appendRow(model, i18n("Operation"), operation);
    appendRow(model, i18n("Resource"), QString::fromUtf8(ntf.resource()));
    auto tagItem = new QStandardItem(i18n("Tag"));
    populateTagTree(tagItem, ntf.tag());
    model->appendRow(tagItem);
}

void NotificationMonitor::populateSubscriptionNtfTree(QStandardItemModel *model, const Akonadi::Protocol::SubscriptionChangeNotification &ntf)
{
    QString operation;
    switch (ntf.operation()) {
    case Akonadi::Protocol::SubscriptionChangeNotification::Add:
        operation = i18n("Add");
        break;
    case Akonadi::Protocol::SubscriptionChangeNotification::Modify:
        operation = i18n("Modify");
        break;
    case Akonadi::Protocol::SubscriptionChangeNotification::Remove:
        operation = i18n("Remove");
        break;
    case Akonadi::Protocol::SubscriptionChangeNotification::InvalidOp:
        operation = i18n("InvalidOp");
        break;
    }
    appendRow(model, i18n("Operation"), operation);
    appendRow(model, i18n("Subscriber"), QString::fromUtf8(ntf.subscriber()));
    appendRow(model, i18n("Monitored Collections"), toString(ntf.collections()));
    appendRow(model, i18n("Monitored Items"), toString(ntf.items()));
    appendRow(model, i18n("Monitored Tags"), toString(ntf.tags()));
    QStringList types;
    const auto typesSet = ntf.types();
    for (const auto &type : typesSet) {
        switch (type) {
        case Akonadi::Protocol::ModifySubscriptionCommand::ItemChanges:
            types.push_back(i18n("Items"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::CollectionChanges:
            types.push_back(i18n("Collections"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::TagChanges:
            types.push_back(i18n("Tags"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::SubscriptionChanges:
            types.push_back(i18n("Subscriptions"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::ChangeNotifications:
            types.push_back(i18n("Changes"));
            break;
        case Akonadi::Protocol::ModifySubscriptionCommand::NoType:
            types.push_back(i18n("No Type"));
            break;
        }
    }
    appendRow(model, i18n("Monitored Types"), types.join(", "_L1));
    appendRow(model, i18n("Monitored Mime Types"), toString(ntf.mimeTypes()));
    appendRow(model, i18n("Monitored Resources"), toString(ntf.resources()));
    appendRow(model, i18n("Ignored Sessions"), toString(ntf.ignoredSessions()));
    appendRow(model, i18n("All Monitored"), toString(ntf.allMonitored()));
    appendRow(model, i18n("Exclusive"), toString(ntf.exclusive()));

    auto item = new QStandardItem(i18n("Item Fetch Scope"));
    const auto ifs = ntf.itemFetchScope();
    appendRow(item, i18n("Requested Parts"), toString(ifs.requestedParts()));
    appendRow(item, i18n("Changed Since"), ifs.changedSince().toString(Qt::ISODateWithMs));
    QString ancestorDepth;
    switch (ifs.ancestorDepth()) {
    case Akonadi::Protocol::ItemFetchScope::NoAncestor:
        ancestorDepth = i18n("No Ancestor");
        break;
    case Akonadi::Protocol::ItemFetchScope::ParentAncestor:
        ancestorDepth = i18n("Parent  Ancestor");
        break;
    case Akonadi::Protocol::ItemFetchScope::AllAncestors:
        ancestorDepth = i18n("All Ancestors");
        break;
    }
    appendRow(item, i18n("Ancestor Depth"), ancestorDepth);
    appendRow(item, i18n("Cache Only"), toString(ifs.cacheOnly()));
    appendRow(item, i18n("Check Cached Payload Parts Only"), toString(ifs.checkCachedPayloadPartsOnly()));
    appendRow(item, i18n("Full Payload"), toString(ifs.fullPayload()));
    appendRow(item, i18n("All Attributes"), toString(ifs.allAttributes()));
    appendRow(item, i18n("Fetch Size"), toString(ifs.fetchSize()));
    appendRow(item, i18n("Fetch MTime"), toString(ifs.fetchMTime()));
    appendRow(item, i18n("Fetch Remote Revision"), toString(ifs.fetchRemoteRevision()));
    appendRow(item, i18n("Ignore Errors"), toString(ifs.ignoreErrors()));
    appendRow(item, i18n("Fetch Flags"), toString(ifs.fetchFlags()));
    appendRow(item, i18n("Fetch RemoteID"), toString(ifs.fetchRemoteId()));
    appendRow(item, i18n("Fetch GID"), toString(ifs.fetchGID()));
    appendRow(item, i18n("Fetch Tags"), toString(ifs.fetchTags()));
    appendRow(item, i18n("Fetch VRefs"), toString(ifs.fetchVirtualReferences()));
    model->appendRow(item);

    item = new QStandardItem(QStringLiteral("Collection Fetch Scope"));
    const auto cfs = ntf.collectionFetchScope();
    QString listFilter;
    switch (cfs.listFilter()) {
    case Akonadi::Protocol::CollectionFetchScope::NoFilter:
        listFilter = i18n("No Filter");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Display:
        listFilter = i18n("Display");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Enabled:
        listFilter = i18n("Enabled");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Index:
        listFilter = i18n("Index");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Sync:
        listFilter = i18n("Sync");
        break;
    }
    appendRow(item, i18n("List Filter"), listFilter);
    appendRow(item, i18n("Include Statistics"), toString(cfs.includeStatistics()));
    appendRow(item, i18n("Resource"), cfs.resource());
    appendRow(item, i18n("Content Mime Types"), cfs.contentMimeTypes().join(", "_L1));
    appendRow(item, i18n("Attributes"), toString(cfs.attributes()));
    appendRow(item, i18n("Fetch ID Only"), toString(cfs.fetchIdOnly()));
    QString ancestorRetrieval;
    switch (cfs.ancestorRetrieval()) {
    case Akonadi::Protocol::CollectionFetchScope::All:
        ancestorRetrieval = i18n("All");
        break;
    case Akonadi::Protocol::CollectionFetchScope::Parent:
        ancestorRetrieval = i18n("Parent");
        break;
    case Akonadi::Protocol::CollectionFetchScope::None:
        ancestorRetrieval = i18n("None");
        break;
    }
    appendRow(item, i18n("Ancestor Retrieval"), ancestorRetrieval);
    appendRow(item, i18n("Ancestor Fetch ID Only"), toString(cfs.ancestorFetchIdOnly()));
    appendRow(item, i18n("Ancestor Attributes"), toString(cfs.ancestorAttributes()));
    appendRow(item, i18n("Ignore Retrieval Errors"), toString(cfs.ignoreRetrievalErrors()));
    model->appendRow(item);

    item = new QStandardItem(i18n("Tag Fetch Scope"));
    const Akonadi::Protocol::TagFetchScope tfs = ntf.tagFetchScope();
    appendRow(item, i18n("Fetch ID Only"), toString(tfs.fetchIdOnly()));
    appendRow(item, i18n("Fetch RemoteID"), toString(tfs.fetchRemoteID()));
    appendRow(item, i18n("Fetch All Attributes"), toString(tfs.fetchAllAttributes()));
    appendRow(item, i18n("Attributes"), toString(tfs.attributes()));
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
        QMessageBox::warning(this, QStringLiteral("Error"), i18n("Failed to open file: %1").arg(file.errorString()));
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

#include "moc_notificationmonitor.cpp"
