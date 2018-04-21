/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "monitorswidget.h"
#include "monitorsmodel.h"
#include "utils.h"

#include <QVBoxLayout>
#include <qheaderview.h>
#include <QStandardItemModel>

#include <AkonadiCore/NotificationSubscriber>
#include <AkonadiWidgets/controlgui.h>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/TagFetchScope>

#include <KSharedConfig>
#include <KConfigGroup>

Q_DECLARE_METATYPE(Akonadi::NotificationSubscriber)

MonitorsWidget::MonitorsWidget(QWidget *parent):
    QWidget(parent)
{
    mModel = new MonitorsModel(this);

    QHBoxLayout *layout = new QHBoxLayout(this);

    mTreeView = new QTreeView(this);
    mTreeView->setModel(mModel);
    mTreeView->setAlternatingRowColors(true);
    mTreeView->setRootIsDecorated(true);
    connect(mTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MonitorsWidget::onSubscriberSelected);
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
            rv << QStringLiteral("Items");
            break;
        case Akonadi::Monitor::Collections:
            rv << QStringLiteral("Collections");
            break;
        case Akonadi::Monitor::Tags:
            rv << QStringLiteral("Tags");
            break;
        case Akonadi::Monitor::Relations:
            rv << QStringLiteral("Relations");
            break;
        case Akonadi::Monitor::Subscribers:
            rv << QStringLiteral("Subscribers");
            break;
        case Akonadi::Monitor::Notifications:
            rv << QStringLiteral("Notification");
            break;
        }
    }
    return rv.join(QStringLiteral(", "));
}


void MonitorsWidget::populateCollectionFetchScope(QStandardItem *parent,
                                                  const Akonadi::CollectionFetchScope &cfs)
{
    appendRow(parent, QStringLiteral("List Filter"), toString(cfs.listFilter()));
    appendRow(parent, QStringLiteral("Include Statistics"), toString(cfs.includeStatistics()));
    appendRow(parent, QStringLiteral("Resource"), cfs.resource());
    appendRow(parent, QStringLiteral("Content MimeTypes"), toString(static_cast<QList<QString>>(cfs.contentMimeTypes())));
    appendRow(parent, QStringLiteral("Ancestor Retrieval"), toString(cfs.ancestorRetrieval()));

    const auto ancestorFs = cfs.ancestorFetchScope();
    if (!ancestorFs.isEmpty()) {
        auto ancestorItem = new QStandardItem(QStringLiteral("Ancestor Fetch Scope"));
        populateCollectionFetchScope(ancestorItem, cfs.ancestorFetchScope());
        parent->appendRow(ancestorItem);
    }

    appendRow(parent, QStringLiteral("Attributes"), toString(cfs.attributes()));
    appendRow(parent, QStringLiteral("Fetch ID Only"), toString(cfs.fetchIdOnly()));
    appendRow(parent, QStringLiteral("Ignore Retrieval Error"), toString(cfs.ignoreRetrievalErrors()));
}

void MonitorsWidget::populateTagFetchScope(QStandardItem *parent,
                                           const Akonadi::TagFetchScope& tfs)
{
    appendRow(parent, QStringLiteral("Attributes"), toString(tfs.attributes()));
    appendRow(parent, QStringLiteral("Fetch ID Only"), toString(tfs.fetchIdOnly()));
}


void MonitorsWidget::onSubscriberSelected(const QModelIndex& index)
{
    auto model = qobject_cast<QStandardItemModel*>(mSubscriberView->model());
    const auto state = mSubscriberView->header()->saveState();
    model->clear();
    model->setHorizontalHeaderLabels({ QStringLiteral("Properties"), QStringLiteral("Values") });
    mSubscriberView->header()->restoreState(state);

    const auto subscriber = index.data(MonitorsModel::SubscriberRole).value<Akonadi::NotificationSubscriber>();
    if (!subscriber.isValid()) {
        return;
    }

    appendRow(model, QStringLiteral("Subscriber"), QString::fromUtf8(subscriber.subscriber()));
    appendRow(model, QStringLiteral("Session ID"), QString::fromUtf8(subscriber.sessionId()));
    appendRow(model, QStringLiteral("Monitored Collections:"), toString(subscriber.monitoredCollections()));
    appendRow(model, QStringLiteral("Monitored Items:"), toString(subscriber.monitoredItems()));
    appendRow(model, QStringLiteral("Monitored Tags:"), toString(subscriber.monitoredTags()));
    appendRow(model, QStringLiteral("Monitored Types:"), toString(subscriber.monitoredTypes()));
    appendRow(model, QStringLiteral("Monitored Mime Types:"), toString(subscriber.monitoredMimeTypes()));
    appendRow(model, QStringLiteral("Monitored Resources:"), toString(subscriber.monitoredResources()));
    appendRow(model, QStringLiteral("Ignored Sessions:"), toString(subscriber.ignoredSessions()));
    appendRow(model, QStringLiteral("All Monitored:"), toString(subscriber.isAllMonitored()));
    appendRow(model, QStringLiteral("Is Exclusive:"), toString(subscriber.isExclusive()));

    auto ifsItem = new QStandardItem(QStringLiteral("Item Fetch Scope"));
    const auto ifs = subscriber.itemFetchScope();
    appendRow(ifsItem, QStringLiteral("Payload Parts"), toString(ifs.payloadParts()));
    appendRow(ifsItem, QStringLiteral("Full Paryload"), toString(ifs.fullPayload()));
    appendRow(ifsItem, QStringLiteral("Attributes"), toString(ifs.attributes()));
    appendRow(ifsItem, QStringLiteral("All Attributes"), toString(ifs.allAttributes()));
    appendRow(ifsItem, QStringLiteral("Cache only"), toString(ifs.cacheOnly()));
    appendRow(ifsItem, QStringLiteral("Check for Cached Payloads Parts Only"),
              toString(ifs.checkForCachedPayloadPartsOnly()));
    appendRow(ifsItem, QStringLiteral("Ancestor Retrieval"), toString(ifs.ancestorRetrieval()));
    appendRow(ifsItem, QStringLiteral("Fetch mtime"), toString(ifs.fetchModificationTime()));
    appendRow(ifsItem, QStringLiteral("Fetch GID"), toString(ifs.fetchGid()));
    appendRow(ifsItem, QStringLiteral("Ignore Retrieval Errors"), toString(ifs.ignoreRetrievalErrors()));
    appendRow(ifsItem, QStringLiteral("Fetch Change Since"), toString(ifs.fetchChangedSince()));
    appendRow(ifsItem, QStringLiteral("Fetch RID"), toString(ifs.fetchRemoteIdentification()));
    appendRow(ifsItem, QStringLiteral("Fetch Tags"), toString(ifs.fetchTags()));

    auto tfsItem = new QStandardItem(QStringLiteral("Tag Fetch Scope"));
    populateTagFetchScope(tfsItem, ifs.tagFetchScope());
    model->appendRow(ifsItem);

    const auto cfs = subscriber.collectionFetchScope();
    auto cfsItem = new QStandardItem(QStringLiteral("Collection Fetch Scope"));
    populateCollectionFetchScope(cfsItem, cfs);

    auto tagScope = new QStandardItem(QStringLiteral("Tag Fetch Scope"));
    populateTagFetchScope(tagScope, ifs.tagFetchScope());
    model->appendRow(tagScope);

    mSubscriberView->expandAll();
}

