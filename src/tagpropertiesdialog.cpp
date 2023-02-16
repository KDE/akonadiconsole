/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "tagpropertiesdialog.h"
#include <Akonadi/DbAccess>

#include <KLocalizedString>

#include <Akonadi/AttributeFactory>
#include <QDialogButtonBox>
#include <QSqlError>
#include <QSqlQuery>

using namespace Akonadi;

TagPropertiesDialog::TagPropertiesDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

TagPropertiesDialog::TagPropertiesDialog(const Akonadi::Tag &tag, QWidget *parent)
    : QDialog(parent)
    , mTag(tag)
    , mChanged(false)
{
    setupUi();
}

TagPropertiesDialog::~TagPropertiesDialog() = default;

Tag TagPropertiesDialog::tag() const
{
    return mTag;
}

bool TagPropertiesDialog::changed() const
{
    return mChanged;
}

void TagPropertiesDialog::setupUi()
{
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TagPropertiesDialog::slotAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TagPropertiesDialog::reject);

    auto widget = new QWidget(this);
    ui.setupUi(widget);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);

    connect(ui.addAttrButton, &QPushButton::clicked, this, &TagPropertiesDialog::addAttributeClicked);
    connect(ui.deleteAttrButton, &QPushButton::clicked, this, &TagPropertiesDialog::deleteAttributeClicked);

    connect(ui.addRIDButton, &QPushButton::clicked, this, &TagPropertiesDialog::addRIDClicked);
    connect(ui.deleteRIDButton, &QPushButton::clicked, this, &TagPropertiesDialog::deleteRIDClicked);

    Attribute::List attributes = mTag.attributes();
    mAttributesModel = new QStandardItemModel(attributes.size(), 2, this);
    connect(mAttributesModel, &QStandardItemModel::itemChanged, this, &TagPropertiesDialog::attributeChanged);
    QStringList labels;
    labels << i18n("Attribute") << i18n("Value");
    mAttributesModel->setHorizontalHeaderLabels(labels);

    mRemoteIdsModel = new QStandardItemModel(this);
    connect(mRemoteIdsModel, &QStandardItemModel::itemChanged, this, &TagPropertiesDialog::remoteIdChanged);
    mRemoteIdsModel->setColumnCount(2);
    labels.clear();
    labels << i18n("Resource") << i18n("Remote ID");
    mRemoteIdsModel->setHorizontalHeaderLabels(labels);
    ui.ridsView->setModel(mRemoteIdsModel);

    if (mTag.isValid()) {
        ui.idLabel->setText(QString::number(mTag.id()));
        ui.typeEdit->setText(QLatin1String(mTag.type()));
        ui.gidEdit->setText(QLatin1String(mTag.gid()));
        ui.parentIdLabel->setText(QString::number(mTag.parent().id()));

        for (int i = 0; i < attributes.count(); ++i) {
            QModelIndex index = mAttributesModel->index(i, 0);
            Q_ASSERT(index.isValid());
            mAttributesModel->setData(index, QLatin1String(attributes[i]->type()));
            mAttributesModel->item(i, 0)->setEditable(false);
            index = mAttributesModel->index(i, 1);
            Q_ASSERT(index.isValid());
            mAttributesModel->setData(index, QLatin1String(attributes[i]->serialized()));
            mAttributesModel->item(i, 1)->setEditable(true);
        }

        {
            // There is (intentionally) no way to retrieve Tag RID for another
            // resource than in the current context. Since Akonadi Console has
            // not resource context at all, we need to retrieve the IDs the hard way
            QSqlQuery query(DbAccess::database());
            query.prepare(
                QStringLiteral("SELECT ResourceTable.name, TagRemoteIdResourceRelationTable.remoteId "
                               "FROM TagRemoteIdResourceRelationTable "
                               "LEFT JOIN ResourceTable ON ResourceTable.id = TagRemoteIdResourceRelationTable.resourceId "
                               "WHERE TagRemoteIdResourceRelationTable.tagid = ?"));
            query.addBindValue(mTag.id());
            if (query.exec()) {
                while (query.next()) {
                    QList<QStandardItem *> items;
                    auto item = new QStandardItem(query.value(0).toString());
                    item->setEditable(false);
                    items << item;
                    item = new QStandardItem(query.value(1).toString());
                    item->setEditable(true);
                    items << item;
                    mRemoteIdsModel->appendRow(items);
                }
            } else {
                qCritical() << query.executedQuery();
                qCritical() << query.lastError().text();
            }
        }
    } else {
        ui.idLabel->setVisible(false);
        ui.idLabelBuddy->setVisible(false);
        if (mTag.parent().isValid()) {
            ui.parentIdLabel->setText(QString::number(mTag.parent().id()));
        } else {
            ui.parentIdLabel->setVisible(false);
            ui.parentIdLabelBuddy->setVisible(false);
        }
        // Since we are using direct SQL to update RIDs, we cannot do this
        // when creating a new Tag, because the tag is created by caller after
        // this dialog is closed
        ui.tabWidget->setTabEnabled(2, false);
    }

    ui.attrsView->setModel(mAttributesModel);
}

void TagPropertiesDialog::addAttributeClicked()
{
    const QString newType = ui.newAttrEdit->text();
    if (newType.isEmpty()) {
        return;
    }
    ui.newAttrEdit->clear();

    mChangedAttrs.insert(newType);
    mRemovedAttrs.remove(newType);
    mChanged = true;

    const int row = mAttributesModel->rowCount();
    mAttributesModel->insertRow(row);
    const QModelIndex index = mAttributesModel->index(row, 0);
    Q_ASSERT(index.isValid());
    mAttributesModel->setData(index, newType);
    mAttributesModel->item(row, 0)->setEditable(false);
    mAttributesModel->setItem(row, 1, new QStandardItem);
    mAttributesModel->item(row, 1)->setEditable(true);
}

void TagPropertiesDialog::deleteAttributeClicked()
{
    const QModelIndexList selection = ui.attrsView->selectionModel()->selectedRows();
    if (selection.count() != 1) {
        return;
    }
    const QString attr = selection.first().data().toString();
    mChangedAttrs.remove(attr);
    mRemovedAttrs.insert(attr);
    mChanged = true;
    mAttributesModel->removeRow(selection.first().row());
}

void TagPropertiesDialog::attributeChanged(QStandardItem *item)
{
    const QString attr = mAttributesModel->data(mAttributesModel->index(item->row(), 0)).toString();
    mRemovedAttrs.remove(attr);
    mChangedAttrs.insert(attr);
    mChanged = true;
}

void TagPropertiesDialog::addRIDClicked()
{
    const QString newResource = ui.newRIDEdit->text();
    if (newResource.isEmpty()) {
        return;
    }
    ui.newRIDEdit->clear();

    mChangedRIDs.insert(newResource);
    mRemovedRIDs.remove(newResource);
    // Don't change mChanged here, we will handle this internally

    const int row = mRemoteIdsModel->rowCount();
    mRemoteIdsModel->insertRow(row);
    const QModelIndex index = mRemoteIdsModel->index(row, 0);
    Q_ASSERT(index.isValid());
    mRemoteIdsModel->setData(index, newResource);
    mRemoteIdsModel->item(row, 0)->setEditable(false);
    mRemoteIdsModel->setItem(row, 1, new QStandardItem);
    mRemoteIdsModel->item(row, 1)->setEditable(true);
}

void TagPropertiesDialog::deleteRIDClicked()
{
    const QModelIndexList selection = ui.ridsView->selectionModel()->selectedRows();
    if (selection.count() != 1) {
        return;
    }
    const QString res = selection.first().data().toString();
    mChangedRIDs.remove(res);
    mRemovedRIDs.insert(res);
    // Don't change mChanged here, we will handle this internally
    mRemoteIdsModel->removeRow(selection.first().row());
}

void TagPropertiesDialog::remoteIdChanged(QStandardItem *item)
{
    const QString res = mRemoteIdsModel->data(mRemoteIdsModel->index(item->row(), 0)).toString();
    mRemovedRIDs.remove(res);
    mChangedRIDs.insert(res);
    // Don't change mChanged here, we will handle this internally
}

void TagPropertiesDialog::slotAccept()
{
    mChanged |= (mTag.type() != ui.typeEdit->text().toLatin1());
    mChanged |= (mTag.gid() != ui.gidEdit->text().toLatin1());

    if (!mChanged && mChangedRIDs.isEmpty() && mRemovedRIDs.isEmpty()) {
        QDialog::accept();
        return;
    }

    mTag.setType(ui.typeEdit->text().toLatin1());
    mTag.setGid(ui.gidEdit->text().toLatin1());

    for (const QString &removedAttr : std::as_const(mRemovedAttrs)) {
        mTag.removeAttribute(removedAttr.toLatin1());
    }
    for (int i = 0; i < mAttributesModel->rowCount(); ++i) {
        const QModelIndex typeIndex = mAttributesModel->index(i, 0);
        Q_ASSERT(typeIndex.isValid());
        if (!mChangedAttrs.contains(typeIndex.data().toString())) {
            continue;
        }
        const QModelIndex valueIndex = mAttributesModel->index(i, 1);
        Attribute *attr = AttributeFactory::createAttribute(mAttributesModel->data(typeIndex).toString().toLatin1());
        if (!attr) {
            continue;
        }
        attr->deserialize(mAttributesModel->data(valueIndex).toString().toLatin1());
        mTag.addAttribute(attr);
    }

    bool queryOK = true;
    if (mTag.isValid() && (!mRemovedRIDs.isEmpty() || !mChangedRIDs.isEmpty())) {
        DbAccess::database().transaction();
    }

    if (mTag.isValid() && !mRemovedRIDs.isEmpty()) {
        QSqlQuery query(DbAccess::database());
        QString queryStr = QStringLiteral(
            "DELETE FROM TagRemoteIdResourceRelationTable "
            "WHERE tagId = ? AND "
            "resourceId IN (SELECT id "
            "FROM ResourceTable "
            "WHERE ");
        QStringList conds;
        for (int i = 0; i < mRemovedRIDs.count(); ++i) {
            conds << QStringLiteral("name = ?");
        }
        queryStr += conds.join(QLatin1String(" OR ")) + QLatin1Char(')');
        query.prepare(queryStr);
        query.addBindValue(mTag.id());
        for (const QString &removedRid : std::as_const(mRemovedRIDs)) {
            query.addBindValue(removedRid);
        }
        if (!query.exec()) {
            qCritical() << query.executedQuery();
            qCritical() << query.lastError().text();
            queryOK = false;
        }
    }
    if (queryOK && mTag.isValid() && !mChangedRIDs.isEmpty()) {
        QMap<QString, qint64> resourceNameToIdMap;
        QVector<qint64> existingResourceRecords;
        {
            QSqlQuery query(DbAccess::database());
            QString queryStr = QStringLiteral("SELECT id, name FROM ResourceTable WHERE ");
            QStringList conds;
            for (int i = 0; i < mChangedRIDs.count(); ++i) {
                conds << QStringLiteral("name = ?");
            }
            queryStr += conds.join(QLatin1String(" OR "));
            query.prepare(queryStr);
            for (const QString &res : std::as_const(mChangedRIDs)) {
                query.addBindValue(res);
            }
            if (!query.exec()) {
                qCritical() << query.executedQuery();
                qCritical() << query.lastError().text();
                queryOK = false;
            }

            while (query.next()) {
                resourceNameToIdMap[query.value(1).toString()] = query.value(0).toLongLong();
            }
        }

        // This is a workaround for PSQL not supporting UPSERTs
        {
            QSqlQuery query(DbAccess::database());
            query.prepare(QStringLiteral("SELECT resourceId FROM TagRemoteIdResourceRelationTable WHERE tagId = ?"));
            query.addBindValue(mTag.id());
            if (!query.exec()) {
                qCritical() << query.executedQuery();
                qCritical() << query.lastError().text();
                queryOK = false;
            }

            while (query.next()) {
                existingResourceRecords << query.value(0).toLongLong();
            }
        }

        for (int i = 0; i < mRemoteIdsModel->rowCount() && queryOK; ++i) {
            const QModelIndex resIndex = mRemoteIdsModel->index(i, 0);
            Q_ASSERT(resIndex.isValid());
            if (!mChangedRIDs.contains(resIndex.data().toString())) {
                continue;
            }
            const QModelIndex valueIndex = mRemoteIdsModel->index(i, 1);

            QSqlQuery query(DbAccess::database());
            const qlonglong resourceId = resourceNameToIdMap[resIndex.data().toString()];
            if (existingResourceRecords.contains(resourceId)) {
                query.prepare(QStringLiteral("UPDATE TagRemoteIdResourceRelationTable SET remoteId = ? WHERE tagId = ? AND resourceId = ?"));
                query.addBindValue(valueIndex.data().toString());
                query.addBindValue(mTag.id());
                query.addBindValue(resourceId);
            } else {
                query.prepare(QStringLiteral("INSERT INTO TagRemoteIdResourceRelationTable (tagId, resourceId, remoteId) VALUES (?, ?, ?)"));
                query.addBindValue(mTag.id());
                query.addBindValue(resourceId);
                query.addBindValue(valueIndex.data().toString());
            }
            if (!query.exec()) {
                qCritical() << query.executedQuery();
                qCritical() << query.lastError().text();
                queryOK = false;
                break;
            }
        }
    }

    if (mTag.isValid() && (!mRemovedRIDs.isEmpty() || !mChangedRIDs.isEmpty())) {
        if (queryOK) {
            DbAccess::database().commit();
        } else {
            DbAccess::database().rollback();
        }
    }

    QDialog::accept();
}
