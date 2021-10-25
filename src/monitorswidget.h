/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#pragma once

#include <QTreeView>
#include <QWidget>

class QStandardItem;
namespace Akonadi
{
class TagFetchScope;
class CollectionFetchScope;
}

class MonitorsModel;

class MonitorsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorsWidget(QWidget *parent = nullptr);
    ~MonitorsWidget() override;

private Q_SLOTS:
    void onSubscriberSelected(const QModelIndex &index);

    void populateTagFetchScope(QStandardItem *parent, const Akonadi::TagFetchScope &tfs);
    void populateCollectionFetchScope(QStandardItem *parent, const Akonadi::CollectionFetchScope &cfs);

private:
    QTreeView *mTreeView = nullptr;
    QTreeView *mSubscriberView = nullptr;
    MonitorsModel *mModel = nullptr;
};

