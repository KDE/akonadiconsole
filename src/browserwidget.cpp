/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browserwidget.h"

#include "akonadibrowsermodel.h"
#include "collectionaclpage.h"
#include "collectionattributespage.h"
#include "collectioninternalspage.h"
#include "config-akonadiconsole.h"
#include "tagpropertiesdialog.h"
#include <Akonadi/ClearCacheFoldersJob>
#include <Akonadi/DbAccess>

#include <Akonadi/AttributeFactory>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionFilterProxyModel>
#include <Akonadi/CollectionPropertiesDialog>
#include <Akonadi/ControlGui>
#include <Akonadi/EntityListView>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeView>
#include <Akonadi/FavoriteCollectionsModel>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/Job>
#include <Akonadi/SelectionProxyModel>
#include <Akonadi/Session>
#include <Akonadi/StandardActionManager>
#include <Akonadi/StatisticsProxyModel>
#include <Akonadi/TagDeleteJob>
#include <Akonadi/TagFetchScope>
#include <Akonadi/TagModel>
#include <Akonadi/XmlWriteJob>
#include <KViewStateMaintainer>
#include <akonadi/private/compressionstream_p.h>

#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/Incidence>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include "akonadiconsole_debug.h"
#include <Akonadi/TagCreateJob>
#include <Akonadi/TagModifyJob>
#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KToggleAction>
#include <KXmlGuiWindow>

#include <KSharedConfig>
#include <QBuffer>
#include <QFileDialog>
#include <QMenu>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>

#if ENABLE_CONTENTVIEWS
#include <Akonadi/ContactGroupViewer>
#include <Akonadi/ContactViewer>
#include <CalendarSupport/IncidenceViewer>
#include <MessageViewer/Viewer>
#endif

using namespace Akonadi;

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionAttributePageFactory, CollectionAttributePage)
AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionInternalsPageFactory, CollectionInternalsPage)
AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionAclPageFactory, CollectionAclPage)

Q_DECLARE_METATYPE(QSet<QByteArray>)

BrowserWidget::BrowserWidget(KXmlGuiWindow *xmlGuiWindow, QWidget *parent)
    : QWidget(parent)
{
    Q_ASSERT(xmlGuiWindow);
    auto layout = new QVBoxLayout(this);

    auto splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("collectionSplitter"));
    layout->addWidget(splitter);

    auto splitter2 = new QSplitter(Qt::Vertical, this);
    splitter2->setObjectName(QStringLiteral("ffvSplitter"));

    mCollectionView = new Akonadi::EntityTreeView(xmlGuiWindow, this);
    mCollectionView->setObjectName(QStringLiteral("CollectionView"));
    mCollectionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    splitter2->addWidget(mCollectionView);

    auto favoritesView = new EntityListView(xmlGuiWindow, this);
    // favoritesView->setViewMode( QListView::IconMode );
    splitter2->addWidget(favoritesView);

    splitter->addWidget(splitter2);

    auto tagRecorder = new ChangeRecorder(this);
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

    auto session = new Session(("AkonadiConsole Browser Widget"), this);

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
    mBrowserMonitor->itemFetchScope().setFetchGid(true);

    mBrowserModel = new AkonadiBrowserModel(mBrowserMonitor, this);
    mBrowserModel->setItemPopulationStrategy(EntityTreeModel::LazyPopulation);
    mBrowserModel->setShowSystemEntities(true);
    mBrowserModel->setListFilter(CollectionFetchScope::Display);

    //   new ModelTest( mBrowserModel );

    auto collectionFilter = new EntityMimeTypeFilterModel(this);
    collectionFilter->setSourceModel(mBrowserModel);
    collectionFilter->addMimeTypeInclusionFilter(Collection::mimeType());
    collectionFilter->setHeaderGroup(EntityTreeModel::CollectionTreeHeaders);
    collectionFilter->setDynamicSortFilter(true);
    collectionFilter->setSortCaseSensitivity(Qt::CaseInsensitive);

    statisticsProxyModel = new Akonadi::StatisticsProxyModel(this);
    statisticsProxyModel->setToolTipEnabled(true);
    statisticsProxyModel->setSourceModel(collectionFilter);

    mCollectionView->setModel(statisticsProxyModel);

    auto selectionProxyModel = new Akonadi::SelectionProxyModel(mCollectionView->selectionModel(), this);
    selectionProxyModel->setSourceModel(mBrowserModel);
    selectionProxyModel->setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);

    auto itemFilter = new EntityMimeTypeFilterModel(this);
    itemFilter->setSourceModel(selectionProxyModel);
    itemFilter->addMimeTypeExclusionFilter(Collection::mimeType());
    itemFilter->setHeaderGroup(EntityTreeModel::ItemListHeaders);

    const KConfigGroup group = KSharedConfig::openConfig()->group(QStringLiteral("FavoriteCollectionsModel"));
    connect(mBrowserModel, &AkonadiBrowserModel::columnsChanged, itemFilter, &EntityMimeTypeFilterModel::invalidate);
    auto sortModel = new AkonadiBrowserSortModel(mBrowserModel, this);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSourceModel(itemFilter);
    auto favoritesModel = new FavoriteCollectionsModel(mBrowserModel, group, this);
    favoritesView->setModel(favoritesModel);

    auto splitter3 = new QSplitter(Qt::Vertical, this);
    splitter3->setObjectName(QStringLiteral("itemSplitter"));
    splitter->addWidget(splitter3);

    auto itemViewParent = new QWidget(this);
    itemUi.setupUi(itemViewParent);

    itemUi.modelBox->addItem(i18n("Generic"));
    itemUi.modelBox->addItem(i18n("Mail"));
    itemUi.modelBox->addItem(i18n("Contacts"));
    itemUi.modelBox->addItem(i18n("Calendar/Tasks"));
    connect(itemUi.modelBox, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &BrowserWidget::modelChanged);
    QTimer::singleShot(0, this, &BrowserWidget::modelChanged);

    itemUi.itemView->setXmlGuiClient(xmlGuiWindow);
    itemUi.itemView->setModel(sortModel);
    itemUi.itemView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(itemUi.itemView->selectionModel(), &QItemSelectionModel::currentChanged, this, &BrowserWidget::currentChanged);

    splitter3->addWidget(itemViewParent);
    itemViewParent->layout()->setContentsMargins(0, 0, 0, 0);

    auto contentViewParent = new QWidget(this);
    contentUi.setupUi(contentViewParent);
    contentUi.saveButton->setEnabled(false);
    connect(contentUi.saveButton, &QPushButton::clicked, this, &BrowserWidget::save);
    splitter3->addWidget(contentViewParent);

#if ENABLE_CONTENTVIEWS
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
    connect(contentUi.gid, &QLineEdit::textChanged, this, &BrowserWidget::contentViewChanged);

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

    mCacheOnlyAction = new KToggleAction(i18n("Cache only retrieval"), xmlGuiWindow);
    mCacheOnlyAction->setChecked(true);
    xmlGuiWindow->actionCollection()->addAction(QStringLiteral("akonadiconsole_cacheonly"), mCacheOnlyAction);
    connect(mCacheOnlyAction, &KToggleAction::toggled, this, &BrowserWidget::updateItemFetchScope);

    m_stateMaintainer = new KViewStateMaintainer<ETMViewStateSaver>(KSharedConfig::openConfig()->group(QStringLiteral("CollectionViewState")), this);
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
    contentUi.gid->clear();
    contentUi.mimeType->clear();
    contentUi.revision->clear();
    contentUi.size->clear();
    contentUi.modificationtime->clear();
    contentUi.flags->clear();
    contentUi.tags->clear();
    contentUi.attrView->setModel(nullptr);
}

void BrowserWidget::currentChanged(const QModelIndex &index)
{
    const Item item = index.sibling(index.row(), 0).data(EntityTreeModel::ItemRole).value<Item>();
    if (!item.isValid()) {
        clear();
        return;
    }

    auto job = new ItemFetchJob(item, this);
    job->fetchScope().fetchFullPayload();
    job->fetchScope().fetchAllAttributes();
    job->fetchScope().setFetchTags(true);
    auto &tfs = job->fetchScope().tagFetchScope();
    tfs.setFetchIdOnly(false);
    tfs.fetchAllAttributes();
    connect(job, &ItemFetchJob::result, this, &BrowserWidget::itemFetchDone);
}

void BrowserWidget::itemFetchDone(KJob *job)
{
    auto fetch = static_cast<ItemFetchJob *>(job);
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
#if ENABLE_CONTENTVIEWS
    if (item.hasPayload<KContacts::Addressee>()) {
        mContactView->setItem(item);
        contentUi.stack->setCurrentWidget(mContactView->parentWidget());
    } else if (item.hasPayload<KContacts::ContactGroup>()) {
        mContactGroupView->setItem(item);
        contentUi.stack->setCurrentWidget(mContactGroupView->parentWidget());
    } else if (item.hasPayload<KCalendarCore::Incidence::Ptr>()) {
        mIncidenceView->setItem(item);
        contentUi.stack->setCurrentWidget(mIncidenceView->parentWidget());
    } else if (item.mimeType() == QLatin1String("message/rfc822") || item.mimeType() == QLatin1String("message/news")) {
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
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);

    if (Akonadi::CompressionStream::isCompressed(&buffer)) {
        Akonadi::CompressionStream stream(&buffer);
        stream.open(QIODevice::ReadOnly);
        data = stream.readAll();
    }

    // Note that this is true for *all* items as soon as the binary format is enabled.
    // Independently from how they are actually stored in the database.
    if (item.hasPayload<KCalendarCore::Incidence::Ptr>()) {
        quint32 magic;
        QDataStream input(data);
        input >> magic;
        KCalendarCore::ICalFormat format;
        if (magic == KCalendarCore::IncidenceBase::magicSerializationIdentifier()) {
            // Binary format isn't readable, show KCalendarCore string instead.
            auto incidence = item.payload<KCalendarCore::Incidence::Ptr>();
            data = "(converted from binary format)\n" + format.toRawString(incidence);
        }
    }

    contentUi.dataView->setPlainText(QString::fromLatin1(data));

    contentUi.id->setText(QString::number(item.id()));
    contentUi.remoteId->setText(item.remoteId());
    contentUi.gid->setText(item.gid());
    contentUi.mimeType->setText(item.mimeType());
    contentUi.revision->setText(QString::number(item.revision()));
    contentUi.size->setText(QString::number(item.size()));
    contentUi.modificationtime->setText(item.modificationTime().toString() + QStringLiteral(" UTC"));
    QStringList flags;
    const auto itemFlags = item.flags();
    for (const Item::Flag &f : itemFlags) {
        flags << QString::fromUtf8(f);
    }
    contentUi.flags->setItems(flags);

    QStringList tags;
    const auto itemTags = item.tags();
    for (const Tag &tag : itemTags) {
        tags << QLatin1String(tag.gid());
    }
    contentUi.tags->setItems(tags);

    Attribute::List list = item.attributes();
    delete mAttrModel;
    mAttrModel = new QStandardItemModel();
    QStringList labels{QStringLiteral("Attribute"), QStringLiteral("Value")};
    mAttrModel->setHorizontalHeaderLabels(labels);
    for (const auto attr : list) {
        auto type = new QStandardItem(QString::fromLatin1(attr->type()));
        type->setEditable(false);
        mAttrModel->appendRow({type, new QStandardItem(QString::fromLatin1(attr->serialized()))});
    }
    contentUi.attrView->setModel(mAttrModel);
    connect(mAttrModel, &QStandardItemModel::itemChanged, this, &BrowserWidget::contentViewChanged);

    if (mMonitor) {
        mMonitor->deleteLater(); // might be the one calling us
    }
    mMonitor = new Monitor(this);
    mMonitor->setObjectName(QStringLiteral("itemMonitor"));
    mMonitor->setItemMonitored(item);
    mMonitor->itemFetchScope().fetchFullPayload();
    mMonitor->itemFetchScope().fetchAllAttributes();
    qRegisterMetaType<QSet<QByteArray>>();
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
    item.setGid(contentUi.gid->text());
    const auto currentItemFlags = mCurrentItem.flags();
    for (const Item::Flag &f : currentItemFlags) {
        item.clearFlag(f);
    }
    const auto contentUiItemFlags = contentUi.flags->items();
    for (const QString &s : contentUiItemFlags) {
        item.setFlag(s.toUtf8());
    }
    const auto contentUiItemTags = mCurrentItem.tags();
    for (const Tag &tag : contentUiItemTags) {
        item.clearTag(tag);
    }
    const auto contentUiTagsItems = contentUi.tags->items();
    for (const QString &s : contentUiTagsItems) {
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

    auto store = new ItemModifyJob(item, this);
    connect(store, &ItemModifyJob::result, this, &BrowserWidget::saveResult);
}

void BrowserWidget::saveResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(this, i18n("Failed to save changes: %1", job->errorString()));
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
    const QString fileName = QFileDialog::getSaveFileName(this, i18n("Select XML file"), QString(), QStringLiteral("*.xml"));
    if (fileName.isEmpty()) {
        return;
    }

    auto job = new XmlWriteJob(root, fileName, this);
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
    auto job = new Akonadi::ClearCacheFoldersJob(currentCollection(), this);
    job->setParentWidget(this);
    job->start();
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
    auto menu = new QMenu(this);
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    menu->addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18n("&Add tag..."), this, &BrowserWidget::addTagRequested);
    if (index.isValid()) {
        menu->addAction(i18n("Add &subtag..."), this, &BrowserWidget::addSubTagRequested);
        menu->addAction(QIcon::fromTheme(QStringLiteral("document-edit")),
                        i18n("&Edit tag..."),
                        QKeySequence(Qt::Key_Return),
                        this,
                        &BrowserWidget::editTagRequested);
        menu->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")),
                        i18n("&Delete tag..."),
                        QKeySequence::Delete,
                        this,
                        &BrowserWidget::removeTagRequested);

        menu->setProperty("Tag", index.data(TagModel::TagRole));
    }

    menu->popup(mTagView->mapToGlobal(pos));
}

void BrowserWidget::addTagRequested()
{
    auto dlg = new TagPropertiesDialog(this);
    dlg->setWindowTitle(i18nc("@title:window", "Add Tag"));
    connect(dlg, &TagPropertiesDialog::accepted, this, &BrowserWidget::createTag);
    connect(dlg, &TagPropertiesDialog::rejected, dlg, &TagPropertiesDialog::deleteLater);
    dlg->show();
}

void BrowserWidget::addSubTagRequested()
{
    auto action = qobject_cast<QAction *>(sender());
    const auto parentTag = action->parent()->property("Tag").value<Akonadi::Tag>();

    Akonadi::Tag tag;
    tag.setParent(parentTag);

    auto dlg = new TagPropertiesDialog(tag, this);
    dlg->setWindowTitle(i18nc("@title:window", "Add Subtag"));
    connect(dlg, &TagPropertiesDialog::accepted, this, &BrowserWidget::createTag);
    connect(dlg, &TagPropertiesDialog::rejected, dlg, &TagPropertiesDialog::deleteLater);
    dlg->show();
}

void BrowserWidget::editTagRequested()
{
    auto action = qobject_cast<QAction *>(sender());
    const auto tag = action->parent()->property("Tag").value<Akonadi::Tag>();
    auto dlg = new TagPropertiesDialog(tag, this);
    dlg->setWindowTitle(i18nc("@title:window", "Modify Tag"));
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

    const auto tag = mTagModel->data(index, TagModel::TagRole).value<Akonadi::Tag>();
    Q_ASSERT(tag.isValid());

    auto dlg = new TagPropertiesDialog(tag, this);
    dlg->setWindowTitle(i18nc("@title:window", "Modify Tag"));
    connect(dlg, &TagPropertiesDialog::accepted, this, &BrowserWidget::modifyTag);
    connect(dlg, &TagPropertiesDialog::rejected, dlg, &TagPropertiesDialog::deleteLater);
    dlg->show();
}

void BrowserWidget::removeTagRequested()
{
    if (KMessageBox::questionTwoActions(this,
                                        i18n("Do you really want to remove selected tag?"),
                                        i18n("Delete tag?"),
                                        KStandardGuiItem::del(),
                                        KStandardGuiItem::cancel(),
                                        QString(),
                                        KMessageBox::Dangerous)
        == KMessageBox::ButtonCode::SecondaryAction) {
        return;
    }

    auto action = qobject_cast<QAction *>(sender());
    const auto tag = action->parent()->property("Tag").value<Akonadi::Tag>();
    new Akonadi::TagDeleteJob(tag, this);
}

void BrowserWidget::createTag()
{
    auto dlg = qobject_cast<TagPropertiesDialog *>(sender());
    Q_ASSERT(dlg);

    if (dlg->changed()) {
        new TagCreateJob(dlg->tag(), this);
    }
}

void BrowserWidget::modifyTag()
{
    auto dlg = qobject_cast<TagPropertiesDialog *>(sender());
    Q_ASSERT(dlg);

    if (dlg->changed()) {
        new TagModifyJob(dlg->tag(), this);
    }
}

#include "moc_browserwidget.cpp"
