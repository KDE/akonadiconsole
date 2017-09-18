/*
    This file is part of Akonadi.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include <xapian.h>

#include "searchwidget.h"
#include "akonadiconsole_debug.h"

#include <AkonadiWidgets/controlgui.h>
#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/itemsearchjob.h>
#include <AkonadiCore/SearchQuery>
#include <kpimtextedit/plaintexteditorwidget.h>


#include <AkonadiSearch/Core/searchstore.h>
#include <AkonadiSearch/Xapian/xapiansearchstore.h>

#include <KComboBox>
#include <KMessageBox>
#include <QTextBrowser>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QSplitter>
#include <QVBoxLayout>

SearchWidget::SearchWidget(QWidget *parent)
    : QWidget(parent)
    , mDatabase(nullptr)
{
    Akonadi::ControlGui::widgetNeedsAkonadi(this);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(QStringLiteral("Search store:")), 0, 0);
    mStoreCombo = new KComboBox;
    mStoreCombo->setObjectName(QStringLiteral("SearchStoreCombo"));
    hbox->addWidget(mStoreCombo);
    hbox->addStretch();
    auto button = new QPushButton(QStringLiteral("Search"));
    hbox->addWidget(button);
    layout->addLayout(hbox);

    mVSplitter = new QSplitter(Qt::Vertical);
    mVSplitter->setObjectName(QStringLiteral("SearchVSplitter"));
    auto w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(QStringLiteral("Search query:")));
    mQueryWidget = new KPIMTextEdit::PlainTextEditorWidget;
    vbox->addWidget(mQueryWidget);
    mVSplitter->addWidget(w);

    mHSplitter = new QSplitter(Qt::Horizontal);
    mHSplitter->setObjectName(QStringLiteral("SearchHSplitter"));
    w = new QWidget;
    vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(QStringLiteral("Results (Documents):")));
    mDatabaseView = new QListView;
    mDatabaseView->setEditTriggers(QListView::NoEditTriggers);
    mDocumentModel = new QStandardItemModel(this);
    mDatabaseView->setModel(mDocumentModel);
    vbox->addWidget(mDatabaseView);
    mHSplitter->addWidget(w);

    w = new QWidget;
    vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(QStringLiteral("Document:")));
    mDocumentView = new QTreeView;
    mDocumentView->setEditTriggers(QTreeView::NoEditTriggers);
    mTermModel = new QStandardItemModel(this);
    mDocumentView->setModel(mTermModel);
    vbox->addWidget(mDocumentView);
    mHSplitter->addWidget(w);

    w = new QWidget;
    vbox = new QVBoxLayout(w);
    vbox->addWidget(new QLabel(QStringLiteral("Item:")));
    mItemView = new QTextBrowser;
    vbox->addWidget(mItemView);
    mHSplitter->addWidget(w);

    mVSplitter->addWidget(mHSplitter);

    layout->addWidget(mVSplitter);


    const auto stores = Akonadi::Search::SearchStore::searchStores();
    for (const auto &store : stores) {
        mStoreCombo->addItem(store->types().last(), QVariant::fromValue(store));
    }

    connect(button, &QPushButton::clicked, this, &SearchWidget::search);
    connect(mDatabaseView, &QListView::activated, this, &SearchWidget::fetchItem);
    connect(mStoreCombo, QOverload<int>::of(&KComboBox::currentIndexChanged), this, &SearchWidget::openStore);

    openStore(0);

    KConfigGroup config(KSharedConfig::openConfig(), "SearchWidget");
    mQueryWidget->setPlainText(config.readEntry("query"));
}

SearchWidget::~SearchWidget()
{
    KConfigGroup config(KSharedConfig::openConfig(), "SearchWidget");
    config.writeEntry("query", mQueryWidget->toPlainText());
    config.sync();
}

void SearchWidget::openStore(int idx)
{
    auto store = mStoreCombo->itemData(idx, Qt::UserRole).value<QSharedPointer<Akonadi::Search::SearchStore>>();
    auto xapianStore = store.objectCast<Akonadi::Search::XapianSearchStore>();
    Q_ASSERT(xapianStore);

    if (mDatabase) {
        mDatabase->close();
        delete mDatabase;
    }

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
    qCWarning(AKONADICONSOLE_LOG) << e.get_type() << QString::fromStdString(e.get_description()) << QString::fromStdString(e.get_error_string());
    QMessageBox::critical(this, QStringLiteral("Xapian error"),
                          QStringLiteral("%1: %2").arg(QString::fromUtf8(e.get_type()), QString::fromStdString(e.get_msg())));
}



void SearchWidget::search()
{
    if (!mDatabase) {
        QMessageBox::critical(this, QStringLiteral("Error"),
                              QStringLiteral("No Xapian database is opened"));
        return;
    }

    mDocumentModel->clear();
    try {
        const auto q = mQueryWidget->toPlainText().toStdString();
        auto it = mDatabase->postlist_begin(q);
        const auto end = mDatabase->postlist_end(q);

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
        mTermModel->setHorizontalHeaderLabels({ QStringLiteral("Term/Value"), QStringLiteral("WDF/Slot") });

        auto termsRoot = new QStandardItem(QStringLiteral("Terms"));
        mTermModel->appendRow(termsRoot);
        for (auto it = doc.termlist_begin(), end = doc.termlist_end(); it != end; ++it) {
            termsRoot->appendRow({ new QStandardItem(QString::fromStdString(*it)),
                                   new QStandardItem(QString::number(it.get_wdf())) });
        }

        auto valuesRoot = new QStandardItem(QStringLiteral("Values"));
        mTermModel->appendRow(valuesRoot);
	const auto end = doc.values_end(); // Xapian 1.2 has different type for _begin() and _end() iters
        for (auto it = doc.values_begin(); it != end; ++it) {
            valuesRoot->appendRow({ new QStandardItem(QString::fromStdString(*it)),
                                    new QStandardItem(QString::number(it.get_valueno())) });
        }
    } catch (const Xapian::Error &e) {
        xapianError(e);
        return;
    }


    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(Akonadi::Item(docId));
    fetchJob->fetchScope().fetchFullPayload();
    connect(fetchJob, &Akonadi::ItemFetchJob::result, this, &SearchWidget::itemFetched);
}

void SearchWidget::itemFetched(KJob *job)
{
    mItemView->clear();

    if (job->error()) {
        KMessageBox::error(this, QStringLiteral("Error on fetching item"));
        return;
    }

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (!fetchJob->items().isEmpty()) {
        const Akonadi::Item item = fetchJob->items().first();
        mItemView->setPlainText(QString::fromUtf8(item.payloadData()));
    }
}
