/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef TAGPROPERTIESDIALOG_H
#define TAGPROPERTIESDIALOG_H

#include <QDialog>
#include <AkonadiCore/tag.h>

#include <QStandardItemModel>

#include "ui_tagpropertiesdialog.h"

class TagPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagPropertiesDialog(QWidget *parent = nullptr);
    explicit TagPropertiesDialog(const Akonadi::Tag &tag, QWidget *parent = nullptr);
    virtual ~TagPropertiesDialog();

    Akonadi::Tag tag() const;
    bool changed() const;

protected:
    void slotAccept();

private Q_SLOTS:
    void addAttributeClicked();
    void deleteAttributeClicked();
    void attributeChanged(QStandardItem *item);

    void addRIDClicked();
    void deleteRIDClicked();
    void remoteIdChanged(QStandardItem *item);

private:
    void setupUi();

    Ui::TagPropertiesDialog ui;
    Akonadi::Tag mTag;

    QStandardItemModel *mAttributesModel = nullptr;
    QStandardItemModel *mRemoteIdsModel = nullptr;

    bool mChanged = false;
    QSet<QString> mChangedAttrs;
    QSet<QString> mRemovedAttrs;
    QSet<QString> mChangedRIDs;
    QSet<QString> mRemovedRIDs;
};

#endif // TAGPROPERTIESDIALOG_H
