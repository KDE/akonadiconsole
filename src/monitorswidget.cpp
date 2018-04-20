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

namespace {

template<typename T, template<typename> class Container>
QString toString(const Container<T> &set)
{
    QStringList rv;
    for (const auto &v : set) {
        rv << QVariant(v).toString();
    }
    return rv.join(QStringLiteral(", "));
}

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

QString toString(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString toString(const QDateTime &dt)
{
    return dt.toString(Qt::ISODate);
}

}

void MonitorsWidget::populateCollectionFetchScope(QStandardItem *parent,
                                                  const Akonadi::CollectionFetchScope &cfs)
{
    parent->appendRow({ new QStandardItem(QStringLiteral("List Filter")),
                         new QStandardItem(toString(cfs.listFilter())) });
    parent->appendRow({ new QStandardItem(QStringLiteral("Include Statistics")),
                         new QStandardItem(toString(cfs.includeStatistics())) });
    parent->appendRow({ new QStandardItem(QStringLiteral("Resource")),
                         new QStandardItem(cfs.resource()) });
    parent->appendRow({ new QStandardItem(QStringLiteral("Content MimeTypes")),
                         new QStandardItem(toString(static_cast<QList<QString>>(cfs.contentMimeTypes()))) });
    parent->appendRow({ new QStandardItem(QStringLiteral("Ancestor Retrieval")),
                         new QStandardItem(toString(cfs.ancestorRetrieval())) });

    const auto ancestorFs = cfs.ancestorFetchScope();
    if (!ancestorFs.isEmpty()) {
        auto ancestorItem = new QStandardItem(QStringLiteral("Ancestor Fetch Scope"));
        populateCollectionFetchScope(ancestorItem, cfs.ancestorFetchScope());
        parent->appendRow(ancestorItem);
    }

    parent->appendRow({ new QStandardItem(QStringLiteral("Attributes")),
                         new QStandardItem(toString(cfs.attributes())) });
    parent->appendRow({ new QStandardItem(QStringLiteral("Fetch ID Only")),
                         new QStandardItem(toString(cfs.fetchIdOnly())) });
    parent->appendRow({ new QStandardItem(QStringLiteral("Ignore Retrieval Error")),
                         new QStandardItem(toString(cfs.ignoreRetrievalErrors())) });

}

void MonitorsWidget::populateTagFetchScope(QStandardItem *parent,
                                           const Akonadi::TagFetchScope& tfs)
{
    parent->appendRow({ new QStandardItem(QStringLiteral("Attributes")),
                        new QStandardItem(toString(tfs.attributes())) });
    parent->appendRow({ new QStandardItem(QStringLiteral("Fetch ID Only")),
                        new QStandardItem(toString(tfs.fetchIdOnly())) });
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

    model->appendRow({ new QStandardItem(QStringLiteral("Subscriber")),
                       new QStandardItem(QString::fromUtf8(subscriber.subscriber())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Session ID")),
                       new QStandardItem(QString::fromUtf8(subscriber.sessionId())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Monitored Collections:")),
                       new QStandardItem(toString(subscriber.monitoredCollections())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Monitored Items:")),
                       new QStandardItem(toString(subscriber.monitoredItems())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Monitored Tags:")),
                       new QStandardItem(toString(subscriber.monitoredTags())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Monitored Types:")),
                       new QStandardItem(toString(subscriber.monitoredTypes())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Monitored Mime Types:")),
                       new QStandardItem(toString(subscriber.monitoredMimeTypes())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Monitored Resources:")), 
                       new QStandardItem(toString(subscriber.monitoredResources())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Ignored Sessions:")),
                       new QStandardItem(toString(subscriber.ignoredSessions())) });
    model->appendRow({ new QStandardItem(QStringLiteral("All Monitored:")),
                       new QStandardItem(toString(subscriber.isAllMonitored())) });
    model->appendRow({ new QStandardItem(QStringLiteral("Is Exclusive:")),
                       new QStandardItem(toString(subscriber.isExclusive())) });

    const auto ifs = subscriber.itemFetchScope();
    auto ifsItem = new QStandardItem(QStringLiteral("ItemFetchScope"));
    model->appendRow(ifsItem);
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Payload Parts")),
                         new QStandardItem(toString(ifs.payloadParts())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Full Paryload")),
                         new QStandardItem(toString(ifs.fullPayload())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Attributes")),
                         new QStandardItem(toString(ifs.attributes())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("All Attributes")),
                         new QStandardItem(toString(ifs.allAttributes())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Cache only")),
                         new QStandardItem(toString(ifs.cacheOnly())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Check for Cached Payloads Parts Only")),
                         new QStandardItem(toString(ifs.checkForCachedPayloadPartsOnly())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Ancestor Retrieval")),
                         new QStandardItem(toString(ifs.ancestorRetrieval())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Fetch mtime")),
                         new QStandardItem(toString(ifs.fetchModificationTime())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Fetch GID")),
                         new QStandardItem(toString(ifs.fetchGid())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Ignore Retrieval Errors")),
                         new QStandardItem(toString(ifs.ignoreRetrievalErrors())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Fetch Change Since")),
                         new QStandardItem(toString(ifs.fetchChangedSince())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Fetch RID")),
                         new QStandardItem(toString(ifs.fetchRemoteIdentification())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Fetch Tags")),
                         new QStandardItem(toString(ifs.fetchTags())) });

    auto tfsItem = new QStandardItem(QStringLiteral("Tag Fetch Scope"));
    populateTagFetchScope(tfsItem, ifs.tagFetchScope());
    ifsItem->appendRow(tfsItem);

    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Fetch VRefs")),
                         new QStandardItem(toString(ifs.fetchVirtualReferences())) });
    ifsItem->appendRow({ new QStandardItem(QStringLiteral("Fetch Relations")),
                         new QStandardItem(toString(ifs.fetchRelations())) });

    const auto cfs = subscriber.collectionFetchScope();
    auto cfsItem = new QStandardItem(QStringLiteral("Collection Fetch Scope"));
    populateCollectionFetchScope(cfsItem, cfs);

    auto tagScope = new QStandardItem(QStringLiteral("Tag Fetch Scope"));
    populateTagFetchScope(tagScope, ifs.tagFetchScope());
    model->appendRow(tagScope);

    mSubscriberView->expandAll();
}

