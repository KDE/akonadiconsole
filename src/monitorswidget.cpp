/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "monitorswidget.h"
#include "monitorsmodel.h"
#include "utils.h"

#include <QHBoxLayout>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <qheaderview.h>

#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ControlGui>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/NotificationSubscriber>
#include <Akonadi/TagFetchScope>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

Q_DECLARE_METATYPE(Akonadi::NotificationSubscriber)

MonitorsWidget::MonitorsWidget(QWidget *parent)
    : QWidget(parent)
    , mModel(new MonitorsModel(this))
{
    auto layout = new QHBoxLayout(this);

    mTreeView = new QTreeView(this);
    auto mProxyModel = new QSortFilterProxyModel(this);
    mProxyModel->setDynamicSortFilter(true);
    mProxyModel->sort(0);
    mProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    mProxyModel->setSourceModel(mModel);
    mTreeView->setModel(mProxyModel);
    mTreeView->setAlternatingRowColors(true);
    mTreeView->setRootIsDecorated(true);
    connect(mTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MonitorsWidget::onSubscriberSelected);
    layout->addWidget(mTreeView);

    mSubscriberView = new QTreeView(this);
    mSubscriberView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mSubscriberView->setModel(new QStandardItemModel(this));
    layout->addWidget(mSubscriberView);

    onSubscriberSelected({});

    Akonadi::ControlGui::widgetNeedsAkonadi(this);

    KConfigGroup config(KSharedConfig::openConfig(), "MonitorsWidget");

    mTreeView->header()->restoreState(config.readEntry<QByteArray>("state", QByteArray()));
    mSubscriberView->header()->restoreState(config.readEntry<QByteArray>("subscriberViewState", QByteArray()));
}

MonitorsWidget::~MonitorsWidget()
{
    KConfigGroup config(KSharedConfig::openConfig(), "MonitorsWidget");
    config.writeEntry("state", mTreeView->header()->saveState());
    config.writeEntry("subscriberViewState", mSubscriberView->header()->saveState());
}

// Specialization of function template from utils.h
template<>
QString toString(const QSet<Akonadi::Monitor::Type> &set)
{
    QStringList rv;
    for (const auto &v : set) {
        switch (v) {
        case Akonadi::Monitor::Items:
            rv << i18n("Items");
            break;
        case Akonadi::Monitor::Collections:
            rv << i18n("Collections");
            break;
        case Akonadi::Monitor::Tags:
            rv << i18n("Tags");
            break;
        case Akonadi::Monitor::Relations:
            rv << i18n("Relations");
            break;
        case Akonadi::Monitor::Subscribers:
            rv << i18n("Subscribers");
            break;
        case Akonadi::Monitor::Notifications:
            rv << i18n("Notification");
            break;
        }
    }
    return rv.join(QLatin1String(", "));
}

void MonitorsWidget::populateCollectionFetchScope(QStandardItem *parent, const Akonadi::CollectionFetchScope &cfs)
{
    appendRow(parent, i18n("List Filter"), toString(cfs.listFilter()));
    appendRow(parent, i18n("Include Statistics"), toString(cfs.includeStatistics()));
    appendRow(parent, i18n("Resource"), cfs.resource());
    appendRow(parent, i18n("Content MimeTypes"), toString(static_cast<QList<QString>>(cfs.contentMimeTypes())));
    appendRow(parent, i18n("Ancestor Retrieval"), toString(cfs.ancestorRetrieval()));

    const auto ancestorFs = cfs.ancestorFetchScope();
    if (!ancestorFs.isEmpty()) {
        auto ancestorItem = new QStandardItem(i18n("Ancestor Fetch Scope"));
        populateCollectionFetchScope(ancestorItem, cfs.ancestorFetchScope());
        parent->appendRow(ancestorItem);
    }

    appendRow(parent, i18n("Attributes"), toString(cfs.attributes()));
    appendRow(parent, i18n("Fetch ID Only"), toString(cfs.fetchIdOnly()));
    appendRow(parent, i18n("Ignore Retrieval Error"), toString(cfs.ignoreRetrievalErrors()));
}

void MonitorsWidget::populateTagFetchScope(QStandardItem *parent, const Akonadi::TagFetchScope &tfs)
{
    appendRow(parent, i18n("Attributes"), toString(tfs.attributes()));
    appendRow(parent, i18n("Fetch ID Only"), toString(tfs.fetchIdOnly()));
}

void MonitorsWidget::onSubscriberSelected(const QModelIndex &index)
{
    auto model = qobject_cast<QStandardItemModel *>(mSubscriberView->model());
    const auto state = mSubscriberView->header()->saveState();
    model->clear();
    model->setHorizontalHeaderLabels({i18n("Properties"), i18n("Values")});
    mSubscriberView->header()->restoreState(state);

    const auto subscriber = index.data(MonitorsModel::SubscriberRole).value<Akonadi::NotificationSubscriber>();
    if (!subscriber.isValid()) {
        return;
    }

    appendRow(model, i18n("Subscriber"), QString::fromUtf8(subscriber.subscriber()));
    appendRow(model, i18n("Session ID"), QString::fromUtf8(subscriber.sessionId()));
    appendRow(model, i18n("Monitored Collections:"), toString(subscriber.monitoredCollections()));
    appendRow(model, i18n("Monitored Items:"), toString(subscriber.monitoredItems()));
    appendRow(model, i18n("Monitored Tags:"), toString(subscriber.monitoredTags()));
    appendRow(model, i18n("Monitored Types:"), toString(subscriber.monitoredTypes()));
    appendRow(model, i18n("Monitored Mime Types:"), toString(subscriber.monitoredMimeTypes()));
    appendRow(model, i18n("Monitored Resources:"), toString(subscriber.monitoredResources()));
    appendRow(model, i18n("Ignored Sessions:"), toString(subscriber.ignoredSessions()));
    appendRow(model, i18n("All Monitored:"), toString(subscriber.isAllMonitored()));
    appendRow(model, i18n("Is Exclusive:"), toString(subscriber.isExclusive()));

    auto ifsItem = new QStandardItem(i18n("Item Fetch Scope"));
    const auto ifs = subscriber.itemFetchScope();
    appendRow(ifsItem, i18n("Payload Parts"), toString(ifs.payloadParts()));
    appendRow(ifsItem, i18n("Full Paryload"), toString(ifs.fullPayload()));
    appendRow(ifsItem, i18n("Attributes"), toString(ifs.attributes()));
    appendRow(ifsItem, i18n("All Attributes"), toString(ifs.allAttributes()));
    appendRow(ifsItem, i18n("Cache only"), toString(ifs.cacheOnly()));
    appendRow(ifsItem, i18n("Check for Cached Payloads Parts Only"), toString(ifs.checkForCachedPayloadPartsOnly()));
    appendRow(ifsItem, i18n("Ancestor Retrieval"), toString(ifs.ancestorRetrieval()));
    appendRow(ifsItem, i18n("Fetch mtime"), toString(ifs.fetchModificationTime()));
    appendRow(ifsItem, i18n("Fetch GID"), toString(ifs.fetchGid()));
    appendRow(ifsItem, i18n("Ignore Retrieval Errors"), toString(ifs.ignoreRetrievalErrors()));
    appendRow(ifsItem, i18n("Fetch Change Since"), toString(ifs.fetchChangedSince()));
    appendRow(ifsItem, i18n("Fetch RID"), toString(ifs.fetchRemoteIdentification()));
    appendRow(ifsItem, i18n("Fetch Tags"), toString(ifs.fetchTags()));

    const auto cfs = subscriber.collectionFetchScope();
    auto cfsItem = new QStandardItem(i18n("Collection Fetch Scope"));
    populateCollectionFetchScope(cfsItem, cfs);

    auto tagScope = new QStandardItem(i18n("Tag Fetch Scope"));
    populateTagFetchScope(tagScope, subscriber.tagFetchScope());
    model->appendRow(tagScope);

    mSubscriberView->expandAll();
}

#include "moc_monitorswidget.cpp"
