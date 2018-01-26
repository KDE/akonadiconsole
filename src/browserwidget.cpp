/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "browserwidget.h"

#include "collectionattributespage.h"
#include "collectioninternalspage.h"
#include "collectionaclpage.h"
#include "dbaccess.h"
#include "akonadibrowsermodel.h"
#include "tagpropertiesdialog.h"
#include "config-akonadiconsole.h"

#include <AkonadiCore/attributefactory.h>
#include <AkonadiCore/changerecorder.h>
#include <AkonadiWidgets/controlgui.h>
#include <AkonadiCore/entitymimetypefiltermodel.h>
#include <AkonadiCore/item.h>
#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/itemmodifyjob.h>
#include <AkonadiCore/job.h>
#include <AkonadiCore/collectionfilterproxymodel.h>
#include <AkonadiWidgets/collectionpropertiesdialog.h>
#include <AkonadiCore/selectionproxymodel.h>
#include <AkonadiCore/session.h>
#include <AkonadiWidgets/standardactionmanager.h>
#include <AkonadiWidgets/entitylistview.h>
#include <AkonadiWidgets/entitytreeview.h>
#include <AkonadiWidgets/etmviewstatesaver.h>
#include <AkonadiCore/favoritecollectionsmodel.h>
#include <AkonadiCore/quotacolorproxymodel.h>
#include <AkonadiCore/tagmodel.h>
#include <AkonadiCore/statisticsproxymodel.h>
#include <AkonadiCore/tagdeletejob.h>
#include <AkonadiXml/XmlWriteJob>
#include <kviewstatemaintainer.h>

#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <KCalCore/Incidence>

#include "akonadiconsole_debug.h"
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kmessagebox.h>
#include <kxmlguiwindow.h>
#include <KToggleAction>
#include <KActionCollection>
#include <AkonadiCore/tagmodifyjob.h>
#include <AkonadiCore/tagcreatejob.h>


#include <QSplitter>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QSqlQuery>
#include <QTimer>
#include <KSharedConfig>
#include <QMenu>
#include <QFileDialog>
#include <QSqlError>

#ifdef ENABLE_CONTENTVIEWS
#include <CalendarSupport/IncidenceViewer>
#include <messageviewer/viewer.h>
#include <Akonadi/Contact/ContactViewer>
#include <Akonadi/Contact/ContactGroupViewer>
#endif

using namespace Akonadi;

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionAttributePageFactory, CollectionAttributePage)
AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionInternalsPageFactory, CollectionInternalsPage)
AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionAclPageFactory, CollectionAclPage)

Q_DECLARE_METATYPE(QSet<QByteArray>)

BrowserWidget::BrowserWidget(KXmlGuiWindow *xmlGuiWindow, QWidget *parent) :
    QWidget(parent),
    mAttrModel(nullptr),
    mStdActionManager(nullptr),
    mMonitor(nullptr),
    mCacheOnlyAction(nullptr)
{
    Q_ASSERT(xmlGuiWindow);
    QVBoxLayout *layout = new QVBoxLayout(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("collectionSplitter"));
    layout->addWidget(splitter);

    QSplitter *splitter2 = new QSplitter(Qt::Vertical, this);
    splitter2->setObjectName(QStringLiteral("ffvSplitter"));

    mCollectionView = new Akonadi::EntityTreeView(xmlGuiWindow, this);
    mCollectionView->setObjectName(QStringLiteral("CollectionView"));
    mCollectionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    splitter2->addWidget(mCollectionView);

    EntityListView *favoritesView = new EntityListView(xmlGuiWindow, this);
    //favoritesView->setViewMode( QListView::IconMode );
    splitter2->addWidget(favoritesView);

    splitter->addWidget(splitter2);

    ChangeRecorder *tagRecorder = new ChangeRecorder(this);
    tagRecorder->setObjectName(QStringLiteral("tagRecorder"));
    tagRecorder->setTypeMonitored(Monitor::Tags);
    tagRecorder->setChangeRecordingEnabled(false);
    mTagView = new QTreeView(this);
    mTagModel = new Akonadi::TagModel(tagRecorder, this);
    mTagView->setModel(mTagModel);
    splitter2->addWidget(mTagView);

    mTagView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mTagView, &QTreeView::customContextMenuRequested, this, &BrowserWidget::tagViewContextMenuRequested);
    connect(mTagView, &QTreeView::doubleClicked, this, &BrowserWidget::tagViewDoubleClicked);

    Session *session = new Session(("AkonadiConsole Browser Widget"), this);

    // monitor collection changes
    mBrowserMonitor = new ChangeRecorder(this);
    mBrowserMonitor->setObjectName(QStringLiteral("collectionMonitor"));
    mBrowserMonitor->setSession(session);
    mBrowserMonitor->setCollectionMonitored(Collection::root());
    mBrowserMonitor->fetchCollection(true);
    mBrowserMonitor->setAllMonitored(true);
    // TODO: Only fetch the envelope etc if possible.
    mBrowserMonitor->itemFetchScope().fetchFullPayload(true);
    mBrowserMonitor->itemFetchScope().setCacheOnly(true);

    mBrowserModel = new AkonadiBrowserModel(mBrowserMonitor, this);
    mBrowserModel->setItemPopulationStrategy(EntityTreeModel::LazyPopulation);
    mBrowserModel->setShowSystemEntities(true);
    mBrowserModel->setListFilter(CollectionFetchScope::Display);

//   new ModelTest( mBrowserModel );

    EntityMimeTypeFilterModel *collectionFilter = new EntityMimeTypeFilterModel(this);
    collectionFilter->setSourceModel(mBrowserModel);
    collectionFilter->addMimeTypeInclusionFilter(Collection::mimeType());
    collectionFilter->setHeaderGroup(EntityTreeModel::CollectionTreeHeaders);
    collectionFilter->setDynamicSortFilter(true);
    collectionFilter->setSortCaseSensitivity(Qt::CaseInsensitive);

    statisticsProxyModel = new Akonadi::StatisticsProxyModel(this);
    statisticsProxyModel->setToolTipEnabled(true);
    statisticsProxyModel->setSourceModel(collectionFilter);

    Akonadi::QuotaColorProxyModel *quotaProxyModel = new Akonadi::QuotaColorProxyModel(this);
    quotaProxyModel->setWarningThreshold(50.0);
    quotaProxyModel->setSourceModel(statisticsProxyModel);

    mCollectionView->setModel(quotaProxyModel);

    Akonadi::SelectionProxyModel *selectionProxyModel = new Akonadi::SelectionProxyModel(mCollectionView->selectionModel(), this);
    selectionProxyModel->setSourceModel(mBrowserModel);
    selectionProxyModel->setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);

    EntityMimeTypeFilterModel *itemFilter = new EntityMimeTypeFilterModel(this);
    itemFilter->setSourceModel(selectionProxyModel);
    itemFilter->addMimeTypeExclusionFilter(Collection::mimeType());
    itemFilter->setHeaderGroup(EntityTreeModel::ItemListHeaders);

    const KConfigGroup group = KSharedConfig::openConfig()->group("FavoriteCollectionsModel");
    connect(mBrowserModel, &AkonadiBrowserModel::columnsChanged, itemFilter, &EntityMimeTypeFilterModel::invalidate);
    AkonadiBrowserSortModel *sortModel = new AkonadiBrowserSortModel(mBrowserModel, this);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSourceModel(itemFilter);
    FavoriteCollectionsModel *favoritesModel = new FavoriteCollectionsModel(mBrowserModel, group, this);
    favoritesView->setModel(favoritesModel);

    QSplitter *splitter3 = new QSplitter(Qt::Vertical, this);
    splitter3->setObjectName(QStringLiteral("itemSplitter"));
    splitter->addWidget(splitter3);

    QWidget *itemViewParent = new QWidget(this);
    itemUi.setupUi(itemViewParent);

    itemUi.modelBox->addItem(QStringLiteral("Generic"));
    itemUi.modelBox->addItem(QStringLiteral("Mail"));
    itemUi.modelBox->addItem(QStringLiteral("Contacts"));
    itemUi.modelBox->addItem(QStringLiteral("Calendar"));
    itemUi.modelBox->addItem(QStringLiteral("Calendar/Tasks"));
    connect(itemUi.modelBox, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &BrowserWidget::modelChanged);
    QTimer::singleShot(0, this, &BrowserWidget::modelChanged);

    itemUi.itemView->setXmlGuiClient(xmlGuiWindow);
    itemUi.itemView->setModel(sortModel);
    itemUi.itemView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(itemUi.itemView, &Akonadi::EntityTreeView::activated, this, &BrowserWidget::itemActivated);
    connect(itemUi.itemView, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)));

    splitter3->addWidget(itemViewParent);
    itemViewParent->layout()->setMargin(0);

    QWidget *contentViewParent = new QWidget(this);
    contentUi.setupUi(contentViewParent);
    contentUi.saveButton->setEnabled(false);
    connect(contentUi.saveButton, &QPushButton::clicked, this, &BrowserWidget::save);
    splitter3->addWidget(contentViewParent);

#ifdef ENABLE_CONTENTVIEWS
    auto w = new QWidget;
    w->setLayout(new QVBoxLayout);
    w->layout()->addWidget(mContactView = new Akonadi::ContactViewer);
    contentUi.stack->addWidget(w);

    w = new QWidget;
    w->setLayout(new QVBoxLayout);
    w->layout()->addWidget(mContactGroupView = new Akonadi::ContactGroupViewer);
    contentUi.stack->addWidget(w);

    w = new QWidget;
    w->setLayout(new QVBoxLayout);
    w->layout()->addWidget(mIncidenceView = new CalendarSupport::IncidenceViewer);
    contentUi.stack->addWidget(w);

    w = new QWidget;
    w->setLayout(new QVBoxLayout);
    w->layout()->addWidget(mMailView = new MessageViewer::Viewer(this));
    contentUi.stack->addWidget(w);
#endif

    connect(contentUi.attrAddButton, &QPushButton::clicked, this, &BrowserWidget::addAttribute);
    connect(contentUi.attrDeleteButton, &QPushButton::clicked, this, &BrowserWidget::delAttribute);
    connect(contentUi.flags, &KEditListWidget::changed, this, &BrowserWidget::contentViewChanged);
    connect(contentUi.tags, &KEditListWidget::changed, this, &BrowserWidget::contentViewChanged);
    connect(contentUi.remoteId, &QLineEdit::textChanged, this, &BrowserWidget::contentViewChanged);

    CollectionPropertiesDialog::registerPage(new CollectionAclPageFactory());
    CollectionPropertiesDialog::registerPage(new CollectionAttributePageFactory());
    CollectionPropertiesDialog::registerPage(new CollectionInternalsPageFactory());

    ControlGui::widgetNeedsAkonadi(this);

    mStdActionManager = new StandardActionManager(xmlGuiWindow->actionCollection(), xmlGuiWindow);
    mStdActionManager->setCollectionSelectionModel(mCollectionView->selectionModel());
    mStdActionManager->setItemSelectionModel(itemUi.itemView->selectionModel());
    mStdActionManager->setFavoriteCollectionsModel(favoritesModel);
    mStdActionManager->setFavoriteSelectionModel(favoritesView->selectionModel());
    mStdActionManager->createAllActions();

    mCacheOnlyAction = new KToggleAction(QStringLiteral("Cache only retrieval"), xmlGuiWindow);
    mCacheOnlyAction->setChecked(true);
    xmlGuiWindow->actionCollection()->addAction(QStringLiteral("akonadiconsole_cacheonly"), mCacheOnlyAction);
    connect(mCacheOnlyAction, &KToggleAction::toggled, this, &BrowserWidget::updateItemFetchScope);

    m_stateMaintainer = new KViewStateMaintainer<ETMViewStateSaver>(KSharedConfig::openConfig()->group("CollectionViewState"), this);
    m_stateMaintainer->setView(mCollectionView);

    m_stateMaintainer->restoreState();
}

BrowserWidget::~BrowserWidget()
{
    m_stateMaintainer->saveState();
}

void BrowserWidget::clear()
{
    contentUi.stack->setCurrentWidget(contentUi.unsupportedTypePage);
    contentUi.dataView->clear();
    contentUi.id->clear();
    contentUi.remoteId->clear();
    contentUi.mimeType->clear();
    contentUi.revision->clear();
    contentUi.size->clear();
    contentUi.modificationtime->clear();
    contentUi.flags->clear();
    contentUi.tags->clear();
    contentUi.attrView->setModel(nullptr);
}

void BrowserWidget::itemActivated(const QModelIndex &index)
{
    const Item item = index.sibling(index.row(), 0).data(EntityTreeModel::ItemRole).value< Item >();
    if (!item.isValid()) {
        clear();
        return;
    }

    ItemFetchJob *job = new ItemFetchJob(item, this);
    job->fetchScope().fetchFullPayload();
    job->fetchScope().fetchAllAttributes();
    job->fetchScope().setFetchTags(true);
    connect(job, &ItemFetchJob::result, this, &BrowserWidget::itemFetchDone);
}

void BrowserWidget::itemFetchDone(KJob *job)
{
    ItemFetchJob *fetch = static_cast<ItemFetchJob *>(job);
    if (job->error()) {
        qCWarning(AKONADICONSOLE_LOG) << "Item fetch failed: " << job->errorString();
    } else if (fetch->items().isEmpty()) {
        qCWarning(AKONADICONSOLE_LOG) << "No item found!";
    } else {
        const Item item = fetch->items().first();
        setItem(item);
    }
}

void BrowserWidget::contentViewChanged()
{
    contentUi.saveButton->setEnabled(true);
}

void BrowserWidget::setItem(const Akonadi::Item &item)
{
    mCurrentItem = item;
#ifdef ENABLE_CONTENTVIEWS
    if (item.hasPayload<KContacts::Addressee>()) {
        mContactView->setItem(item);
        contentUi.stack->setCurrentWidget(mContactView->parentWidget());
    } else if (item.hasPayload<KContacts::ContactGroup>()) {
        mContactGroupView->setItem(item);
        contentUi.stack->setCurrentWidget(mContactGroupView->parentWidget());
    } else if (item.hasPayload<KCalCore::Incidence::Ptr>()) {
        mIncidenceView->setItem(item);
        contentUi.stack->setCurrentWidget(mIncidenceView->parentWidget());
    } else if (item.mimeType() == QLatin1String("message/rfc822")
               || item.mimeType() == QLatin1String("message/news")) {
        mMailView->setMessageItem(item, MimeTreeParser::Force);
        contentUi.stack->setCurrentWidget(mMailView->parentWidget());
    } else
#endif
    if (item.hasPayload<QPixmap>()) {
        contentUi.imageView->setPixmap(item.payload<QPixmap>());
        contentUi.stack->setCurrentWidget(contentUi.imageViewPage);
    } else {
        contentUi.stack->setCurrentWidget(contentUi.unsupportedTypePage);
    }

    contentUi.saveButton->setEnabled(false);

    QByteArray data = item.payloadData();
    contentUi.dataView->setPlainText(QString::fromLatin1(data));

    contentUi.id->setText(QString::number(item.id()));
    contentUi.remoteId->setText(item.remoteId());
    contentUi.mimeType->setText(item.mimeType());
    contentUi.revision->setText(QString::number(item.revision()));
    contentUi.size->setText(QString::number(item.size()));
    contentUi.modificationtime->setText(item.modificationTime().toString() + QStringLiteral(" UTC"));
    QStringList flags;
    foreach (const Item::Flag &f, item.flags()) {
        flags << QString::fromUtf8(f);
    }
    contentUi.flags->setItems(flags);

    QStringList tags;
    foreach (const Tag &tag, item.tags()) {
        tags << QLatin1String(tag.gid());
    }
    contentUi.tags->setItems(tags);

    Attribute::List list = item.attributes();
    delete mAttrModel;
    mAttrModel = new QStandardItemModel(list.count(), 2);
    QStringList labels;
    labels << QStringLiteral("Attribute") << QStringLiteral("Value");
    mAttrModel->setHorizontalHeaderLabels(labels);
    for (int i = 0; i < list.count(); ++i) {
        QModelIndex index = mAttrModel->index(i, 0);
        Q_ASSERT(index.isValid());
        mAttrModel->setData(index, QString::fromLatin1(list[i]->type()));
        index = mAttrModel->index(i, 1);
        Q_ASSERT(index.isValid());
        mAttrModel->setData(index, QString::fromLatin1(list[i]->serialized()));
        mAttrModel->itemFromIndex(index)->setFlags(Qt::ItemIsEditable | mAttrModel->flags(index));
    }
    contentUi.attrView->setModel(mAttrModel);

    if (mMonitor) {
        mMonitor->deleteLater();    // might be the one calling us
    }
    mMonitor = new Monitor(this);
    mMonitor->setObjectName(QStringLiteral("itemMonitor"));
    mMonitor->setItemMonitored(item);
    mMonitor->itemFetchScope().fetchFullPayload();
    mMonitor->itemFetchScope().fetchAllAttributes();
    qRegisterMetaType<QSet<QByteArray> >();
    connect(mMonitor, &Akonadi::Monitor::itemChanged, this, &BrowserWidget::setItem, Qt::QueuedConnection);
    contentUi.saveButton->setEnabled(false);
}

void BrowserWidget::modelChanged()
{
    switch (itemUi.modelBox->currentIndex()) {
    case 1:
        mBrowserModel->setItemDisplayMode(AkonadiBrowserModel::MailMode);
        break;
    case 2:
        mBrowserModel->setItemDisplayMode(AkonadiBrowserModel::ContactsMode);
        break;
    case 3:
        mBrowserModel->setItemDisplayMode(AkonadiBrowserModel::CalendarMode);
        break;
    default:
        mBrowserModel->setItemDisplayMode(AkonadiBrowserModel::GenericMode);
    }
}

void BrowserWidget::save()
{
    Q_ASSERT(mAttrModel);

    const QByteArray data = contentUi.dataView->toPlainText().toUtf8();
    Item item = mCurrentItem;
    item.setRemoteId(contentUi.remoteId->text());
    foreach (const Item::Flag &f, mCurrentItem.flags()) {
        item.clearFlag(f);
    }
    foreach (const QString &s, contentUi.flags->items()) {
        item.setFlag(s.toUtf8());
    }
    foreach (const Tag &tag, mCurrentItem.tags()) {
        item.clearTag(tag);
    }
    foreach (const QString &s, contentUi.tags->items()) {
        Tag tag;
        tag.setGid(s.toLatin1());
        item.setTag(tag);
    }
    item.setPayloadFromData(data);

    item.clearAttributes();
    for (int i = 0; i < mAttrModel->rowCount(); ++i) {
        const QModelIndex typeIndex = mAttrModel->index(i, 0);
        Q_ASSERT(typeIndex.isValid());
        const QModelIndex valueIndex = mAttrModel->index(i, 1);
        Q_ASSERT(valueIndex.isValid());
        Attribute *attr = AttributeFactory::createAttribute(mAttrModel->data(typeIndex).toString().toLatin1());
        Q_ASSERT(attr);
        attr->deserialize(mAttrModel->data(valueIndex).toString().toLatin1());
        item.addAttribute(attr);
    }

    ItemModifyJob *store = new ItemModifyJob(item, this);
    connect(store, &ItemModifyJob::result, this, &BrowserWidget::saveResult);
}

void BrowserWidget::saveResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(this, QStringLiteral("Failed to save changes: %1").arg(job->errorString()));
    } else {
        contentUi.saveButton->setEnabled(false);
    }
}

void BrowserWidget::addAttribute()
{
    if (!mAttrModel || contentUi.attrName->text().isEmpty()) {
        return;
    }
    const int row = mAttrModel->rowCount();
    mAttrModel->insertRow(row);
    QModelIndex index = mAttrModel->index(row, 0);
    Q_ASSERT(index.isValid());
    mAttrModel->setData(index, contentUi.attrName->text());
    contentUi.attrName->clear();
    contentUi.saveButton->setEnabled(true);
}

void BrowserWidget::delAttribute()
{
    if (!mAttrModel) {
        return;
    }
    QModelIndexList selection = contentUi.attrView->selectionModel()->selectedRows();
    if (selection.count() != 1) {
        return;
    }
    mAttrModel->removeRow(selection.first().row());
    contentUi.saveButton->setEnabled(true);
}

void BrowserWidget::dumpToXml()
{
    const Collection root = currentCollection();
    if (!root.isValid()) {
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("Select XML file"), QString(), QStringLiteral("*.xml"));
    if (fileName.isEmpty()) {
        return;
    }

    XmlWriteJob *job = new XmlWriteJob(root, fileName, this);
    connect(job, &XmlWriteJob::result, this, &BrowserWidget::dumpToXmlResult);
}

void BrowserWidget::dumpToXmlResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(this, job->errorString());
    }
}

void BrowserWidget::clearCache()
{
    const Collection coll = currentCollection();
    if (!coll.isValid()) {
        return;
    }

    const auto ridCount = QStringLiteral("SELECT COUNT(*) FROM PimItemTable WHERE collectionId=%1 AND remoteId=''").arg(coll.id());
    QSqlQuery query(DbAccess::database());
    if (!query.exec(ridCount)) {
        qCWarning(AKONADICONSOLE_LOG) << "Failed to execute query" << ridCount << ":" << query.lastError().text();
        KMessageBox::error(this, query.lastError().text());
        return;
    }

    query.next();
    const int emptyRidCount = query.value(0).toInt();
    if (emptyRidCount > 0) {
        if (KMessageBox::warningContinueCancel(this, QStringLiteral(
                "The collection '%1' contains %2 items without Remote ID. "
                "Those items were likely never uploaded to the destination server, "
                "so clearing this collection means that those <b>data will be lost</b>. "
                "Are you sure you want to proceed?").arg(coll.id()).arg(emptyRidCount),
                QStringLiteral("POSSIBLE DATA LOSS!")) == KMessageBox::Cancel) {
            return;
        }
    }

    QString str = QStringLiteral("DELETE FROM PimItemTable WHERE collectionId=%1").arg(coll.id());
    qCDebug(AKONADICONSOLE_LOG) << str;
    query = QSqlQuery(str, DbAccess::database());
    if (query.exec()) {
        if (query.lastError().isValid()) {
            qCDebug(AKONADICONSOLE_LOG) << query.lastError();
            KMessageBox::error(this, query.lastError().text());
        }
    }

    // TODO: Clear external parts
    // TODO: Reset Akonadi's internall collection statistics cache
    // TODO: Notify all clients EXCEPT FOR THE RESOURCE about the deletion?
    // TODO: Clear search index
    // TODO: ???

    KMessageBox::information(this, QStringLiteral("Collection cache cleared. You should restart Akonadi now."));
}

Akonadi::Collection BrowserWidget::currentCollection() const
{
    return mCollectionView->currentIndex().data(EntityTreeModel::CollectionRole).value<Collection>();
}

void BrowserWidget::updateItemFetchScope()
{
    mBrowserMonitor->itemFetchScope().setCacheOnly(mCacheOnlyAction->isChecked());
}

void BrowserWidget::tagViewContextMenuRequested(const QPoint &pos)
{
    const QModelIndex index = mTagView->indexAt(pos);
    QMenu *menu = new QMenu(this);
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    menu->addAction(QIcon::fromTheme(QStringLiteral("list-add")), QStringLiteral("&Add tag..."), this, &BrowserWidget::addTagRequested);
    if (index.isValid()) {
        menu->addAction(QStringLiteral("Add &subtag..."), this, &BrowserWidget::addSubTagRequested);
        menu->addAction(QIcon::fromTheme(QStringLiteral("document-edit")), QStringLiteral("&Edit tag..."), this, &BrowserWidget::editTagRequested, QKeySequence(Qt::Key_Return));
        menu->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), QStringLiteral("&Delete tag..."), this, &BrowserWidget::removeTagRequested, QKeySequence::Delete);
        menu->setProperty("Tag", index.data(TagModel::TagRole));
    }

    menu->popup(mTagView->mapToGlobal(pos));
}

void BrowserWidget::addTagRequested()
{
    TagPropertiesDialog *dlg = new TagPropertiesDialog(this);
    connect(dlg, &TagPropertiesDialog::accepted, this, &BrowserWidget::createTag);
    connect(dlg, &TagPropertiesDialog::rejected, dlg, &TagPropertiesDialog::deleteLater);
    dlg->show();
}

void BrowserWidget::addSubTagRequested()
{
    QAction *action = qobject_cast<QAction *>(sender());
    const Akonadi::Tag parentTag = action->parent()->property("Tag").value<Akonadi::Tag>();

    Akonadi::Tag tag;
    tag.setParent(parentTag);

    TagPropertiesDialog *dlg = new TagPropertiesDialog(tag, this);
    connect(dlg, &TagPropertiesDialog::accepted, this, &BrowserWidget::createTag);
    connect(dlg, &TagPropertiesDialog::rejected, dlg, &TagPropertiesDialog::deleteLater);
    dlg->show();
}

void BrowserWidget::editTagRequested()
{
    QAction *action = qobject_cast<QAction *>(sender());
    const Akonadi::Tag tag = action->parent()->property("Tag").value<Akonadi::Tag>();
    TagPropertiesDialog *dlg = new TagPropertiesDialog(tag, this);
    connect(dlg, &TagPropertiesDialog::accepted, this, &BrowserWidget::modifyTag);
    connect(dlg, &TagPropertiesDialog::rejected, dlg, &TagPropertiesDialog::deleteLater);
    dlg->show();
}

void BrowserWidget::tagViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        addTagRequested();
        return;
    }

    const Akonadi::Tag tag = mTagModel->data(index, TagModel::TagRole).value<Akonadi::Tag>();
    Q_ASSERT(tag.isValid());

    TagPropertiesDialog *dlg = new TagPropertiesDialog(tag, this);
    connect(dlg, &TagPropertiesDialog::accepted, this, &BrowserWidget::modifyTag);
    connect(dlg, &TagPropertiesDialog::rejected, dlg, &TagPropertiesDialog::deleteLater);
    dlg->show();
}

void BrowserWidget::removeTagRequested()
{
    if (KMessageBox::questionYesNo(this, QStringLiteral("Do you really want to remove selected tag?"),
                                   QStringLiteral("Delete tag?"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
                                   QString(), KMessageBox::Dangerous) == KMessageBox::No) {
        return;
    }

    QAction *action = qobject_cast<QAction *>(sender());
    const Akonadi::Tag tag = action->parent()->property("Tag").value<Akonadi::Tag>();
    new Akonadi::TagDeleteJob(tag, this);
}

void BrowserWidget::createTag()
{
    TagPropertiesDialog *dlg = qobject_cast<TagPropertiesDialog *>(sender());
    Q_ASSERT(dlg);

    if (dlg->changed()) {
        new TagCreateJob(dlg->tag(), this);
    }
}

void BrowserWidget::modifyTag()
{
    TagPropertiesDialog *dlg = qobject_cast<TagPropertiesDialog *>(sender());
    Q_ASSERT(dlg);

    if (dlg->changed()) {
        new TagModifyJob(dlg->tag(), this);
    }
}
