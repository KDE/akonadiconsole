/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <xapian.h>

#include "akonadiconsole_debug.h"
#include "searchwidget.h"

#include <Akonadi/ControlGui>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemSearchJob>
#include <Akonadi/SearchQuery>

#include <Core/SearchStore>
#include <Xapian/xapiansearchstore.h>

#include <KComboBox>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <QPlainTextEdit>
#include <QTextBrowser>

#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

SearchWidget::SearchWidget(QWidget *parent)
    : QWidget(parent)
{
    Akonadi::ControlGui::widgetNeedsAkonadi(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    auto hbox = new QHBoxLayout;

    hbox->addWidget(new QLabel(i18n("Search store:"), this));
    mStoreCombo = new KComboBox(this);
    mStoreCombo->setObjectName(QStringLiteral("SearchStoreCombo"));
    hbox->addWidget(mStoreCombo);
    hbox->addStretch();
    auto button = new QPushButton(i18n("Search"), this);
    hbox->addWidget(button);
    layout->addLayout(hbox);

    mVSplitter = new QSplitter(Qt::Vertical);
    mVSplitter->setObjectName(QStringLiteral("SearchVSplitter"));
    auto w = new QWidget;
    auto vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(i18n("Search query:"), this));
    mQueryWidget = new QPlainTextEdit;
    vbox->addWidget(mQueryWidget);
    mVSplitter->addWidget(w);

    mHSplitter = new QSplitter(Qt::Horizontal);
    mHSplitter->setObjectName(QStringLiteral("SearchHSplitter"));
    w = new QWidget;
    vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(i18n("Results (Documents):"), this));
    mDatabaseView = new QListView(this);
    mDatabaseView->setEditTriggers(QListView::NoEditTriggers);
    mDocumentModel = new QStandardItemModel(this);
    mDatabaseView->setModel(mDocumentModel);
    vbox->addWidget(mDatabaseView);
    mHSplitter->addWidget(w);

    w = new QWidget;
    vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(i18n("Document:")));
    mDocumentView = new QTreeView;
    mDocumentView->setEditTriggers(QTreeView::NoEditTriggers);
    mTermModel = new QStandardItemModel(this);
    mDocumentView->setModel(mTermModel);
    vbox->addWidget(mDocumentView);
    mHSplitter->addWidget(w);

    w = new QWidget;
    vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(i18n("Item:")));
    mItemView = new QTextBrowser;
    vbox->addWidget(mItemView);
    mHSplitter->addWidget(w);

    mVSplitter->addWidget(mHSplitter);

    layout->addWidget(mVSplitter);

    const auto stores = Akonadi::Search::SearchStore::searchStores();
    qDebug() << " stores " << stores.count();
    for (const auto &store : stores) {
        mStoreCombo->addItem(store->types().last(), QVariant::fromValue(store));
    }

    connect(button, &QPushButton::clicked, this, &SearchWidget::search);
    connect(mDatabaseView, &QListView::activated, this, &SearchWidget::fetchItem);
    connect(mStoreCombo, &KComboBox::currentIndexChanged, this, &SearchWidget::openStore);

    openStore(0);

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("SearchWidget"));
    mQueryWidget->setPlainText(config.readEntry("query"));
}

SearchWidget::~SearchWidget()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("SearchWidget"));
    config.writeEntry("query", mQueryWidget->toPlainText());
    config.sync();
    closeDataBase();
}

void SearchWidget::closeDataBase()
{
    if (mDatabase) {
        mDatabase->close();
        delete mDatabase;
    }
}

void SearchWidget::openStore(int idx)
{
    auto store = mStoreCombo->itemData(idx, Qt::UserRole).value<QSharedPointer<Akonadi::Search::SearchStore>>();
    auto xapianStore = store.objectCast<Akonadi::Search::XapianSearchStore>();
    Q_ASSERT(xapianStore);

    closeDataBase();

    try {
        qCDebug(AKONADICONSOLE_LOG) << "Opening store" << xapianStore->dbPath();
        mDatabase = new Xapian::Database(xapianStore->dbPath().toStdString());
    } catch (Xapian::Error &e) {
        xapianError(e);
        delete mDatabase;
        mDatabase = nullptr;
    }
}

void SearchWidget::xapianError(const Xapian::Error &e)
{
    qCWarning(AKONADICONSOLE_LOG) << e.get_type() << QString::fromStdString(e.get_description()) << e.get_error_string();
    QMessageBox::critical(this, i18n("Xapian error"), QStringLiteral("%1: %2").arg(QString::fromUtf8(e.get_type()), QString::fromStdString(e.get_msg())));
}

void SearchWidget::search()
{
    if (!mDatabase) {
        QMessageBox::critical(this, i18n("Error"), i18n("No Xapian database is opened"));
        return;
    }

    mDocumentModel->clear();
    try {
        const auto q = mQueryWidget->toPlainText().toStdString();
        auto it = mDatabase->postlist_begin(q);
        const auto end = mDatabase->postlist_end(q);

        if (it == end) {
            QMessageBox::information(this, i18n("Search"), i18n("No element found."));
        }
        for (; it != end; ++it) {
            auto item = new QStandardItem(QString::number(*it));
            item->setData(*it, Qt::UserRole);
            mDocumentModel->appendRow(item);
        }
    } catch (Xapian::Error &e) {
        xapianError(e);
        return;
    }
}

void SearchWidget::fetchItem(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const auto docId = index.data(Qt::UserRole).value<Xapian::docid>();

    try {
        const auto doc = mDatabase->get_document(docId);

        mTermModel->clear();
        mTermModel->setColumnCount(2);
        mTermModel->setHorizontalHeaderLabels({i18n("Term/Value"), i18n("WDF/Slot")});

        auto termsRoot = new QStandardItem(i18n("Terms"));
        mTermModel->appendRow(termsRoot);
        for (auto it = doc.termlist_begin(), end = doc.termlist_end(); it != end; ++it) {
            termsRoot->appendRow({new QStandardItem(QString::fromStdString(*it)), new QStandardItem(QString::number(it.get_wdf()))});
        }

        auto valuesRoot = new QStandardItem(i18n("Values"));
        mTermModel->appendRow(valuesRoot);
        const auto end = doc.values_end(); // Xapian 1.2 has different type for _begin() and _end() iters
        for (auto it = doc.values_begin(); it != end; ++it) {
            valuesRoot->appendRow({new QStandardItem(QString::fromStdString(*it)), new QStandardItem(QString::number(it.get_valueno()))});
        }
    } catch (const Xapian::Error &e) {
        xapianError(e);
        return;
    }

    auto fetchJob = new Akonadi::ItemFetchJob(Akonadi::Item(docId));
    fetchJob->fetchScope().fetchFullPayload();
    connect(fetchJob, &Akonadi::ItemFetchJob::result, this, &SearchWidget::itemFetched);
}

void SearchWidget::itemFetched(KJob *job)
{
    mItemView->clear();

    if (job->error()) {
        KMessageBox::error(this, i18n("Error on fetching item"));
        return;
    }

    auto fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (!fetchJob->items().isEmpty()) {
        const Akonadi::Item item = fetchJob->items().first();
        mItemView->setPlainText(QString::fromUtf8(item.payloadData()));
    }
}

#include "moc_searchwidget.cpp"
