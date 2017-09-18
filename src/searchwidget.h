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

#ifndef AKONADICONSOLE_SEARCHWIDGET_H
#define AKONADICONSOLE_SEARCHWIDGET_H

#include <QWidget>

class KComboBox;
class KJob;
class QTextBrowser;

class QListView;
class QModelIndex;
class QStandardItemModel;
class QTreeView;
class QSplitter;
namespace KPIMTextEdit
{
class PlainTextEditorWidget;
}
namespace Xapian {
class Database;
class Error;
}

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent = nullptr);
    ~SearchWidget();

private Q_SLOTS:
    void search();
    void openStore(int idx);
    void fetchItem(const QModelIndex &);
    void itemFetched(KJob *);

private:
    void xapianError(const Xapian::Error &e);
    
    KComboBox *mStoreCombo = nullptr;
    QTextBrowser *mItemView = nullptr;
    KPIMTextEdit::PlainTextEditorWidget *mQueryWidget = nullptr;
    QListView *mDatabaseView = nullptr;
    QStandardItemModel *mDocumentModel = nullptr;
    QTreeView *mDocumentView = nullptr;
    QStandardItemModel *mTermModel = nullptr;
    QSplitter *mHSplitter = nullptr;
    QSplitter *mVSplitter = nullptr;

    Xapian::Database *mDatabase = nullptr;
};

#endif
