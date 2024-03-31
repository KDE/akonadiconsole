/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class KComboBox;
class KJob;
class QTextBrowser;

class QListView;
class QModelIndex;
class QStandardItemModel;
class QTreeView;
class QSplitter;
class QPlainTextEdit;

namespace Xapian
{
class Database;
class Error;
}

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent = nullptr);
    ~SearchWidget() override;

private:
    void search();
    void openStore(int idx);
    void fetchItem(const QModelIndex &);
    void itemFetched(KJob *);
    void xapianError(const Xapian::Error &e);
    void closeDataBase();

    KComboBox *mStoreCombo = nullptr;
    QTextBrowser *mItemView = nullptr;
    QPlainTextEdit *mQueryWidget = nullptr;
    QListView *mDatabaseView = nullptr;
    QStandardItemModel *mDocumentModel = nullptr;
    QTreeView *mDocumentView = nullptr;
    QStandardItemModel *mTermModel = nullptr;
    QSplitter *mHSplitter = nullptr;
    QSplitter *mVSplitter = nullptr;

    Xapian::Database *mDatabase = nullptr;
};
