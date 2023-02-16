/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionattributespage.h"

#include <KLocalizedString>

#include <Akonadi/AttributeFactory>
#include <Akonadi/Collection>

#include <QStandardItemModel>

using namespace Akonadi;

CollectionAttributePage::CollectionAttributePage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setPageTitle(i18n("Attributes"));
    ui.setupUi(this);

    connect(ui.addButton, &QPushButton::clicked, this, &CollectionAttributePage::addAttribute);
    connect(ui.deleteButton, &QPushButton::clicked, this, &CollectionAttributePage::delAttribute);
}

void CollectionAttributePage::load(const Collection &col)
{
    const Attribute::List list = col.attributes();
    mModel = new QStandardItemModel(list.count(), 2);
    QStringList labels;
    labels << i18n("Attribute") << i18n("Value");
    mModel->setHorizontalHeaderLabels(labels);

    for (int i = 0; i < list.count(); ++i) {
        QModelIndex index = mModel->index(i, 0);
        Q_ASSERT(index.isValid());
        mModel->setData(index, QString::fromLatin1(list.at(i)->type()));
        mModel->item(i, 0)->setEditable(false);
        index = mModel->index(i, 1);
        Q_ASSERT(index.isValid());
        mModel->setData(index, QString::fromLatin1(list.at(i)->serialized()));
        mModel->itemFromIndex(index)->setFlags(Qt::ItemIsEditable | mModel->flags(index));
    }
    ui.attrView->setModel(mModel);
    connect(mModel, &QStandardItemModel::itemChanged, this, &CollectionAttributePage::attributeChanged);
}

void CollectionAttributePage::save(Collection &col)
{
    for (const QString &del : std::as_const(mDeleted)) {
        col.removeAttribute(del.toLatin1());
    }
    for (int i = 0; i < mModel->rowCount(); ++i) {
        const QModelIndex typeIndex = mModel->index(i, 0);
        Q_ASSERT(typeIndex.isValid());
        if (!mChanged.contains(typeIndex.data().toString())) {
            continue;
        }
        const QModelIndex valueIndex = mModel->index(i, 1);
        Q_ASSERT(valueIndex.isValid());
        Attribute *attr = AttributeFactory::createAttribute(mModel->data(typeIndex).toString().toLatin1());
        Q_ASSERT(attr);
        attr->deserialize(mModel->data(valueIndex).toString().toLatin1());
        col.addAttribute(attr);
    }
}

void CollectionAttributePage::addAttribute()
{
    if (ui.attrName->text().isEmpty()) {
        return;
    }
    const QString attr = ui.attrName->text();
    mChanged.insert(attr);
    mDeleted.remove(attr);
    const int row = mModel->rowCount();
    mModel->insertRow(row);
    QModelIndex index = mModel->index(row, 0);
    Q_ASSERT(index.isValid());
    mModel->setData(index, attr);
    ui.attrName->clear();
}

void CollectionAttributePage::delAttribute()
{
    QModelIndexList selection = ui.attrView->selectionModel()->selectedRows();
    if (selection.count() != 1) {
        return;
    }
    const QString attr = selection.first().data().toString();
    mChanged.remove(attr);
    mDeleted.insert(attr);
    mModel->removeRow(selection.first().row());
}

void CollectionAttributePage::attributeChanged(QStandardItem *item)
{
    const QString attr = mModel->data(mModel->index(item->row(), 0)).toString();
    mDeleted.remove(attr);
    mChanged.insert(attr);
}
