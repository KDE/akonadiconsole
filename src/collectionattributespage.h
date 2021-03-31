/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ui_collectionattributespage.h"

#include <AkonadiWidgets/collectionpropertiespage.h>

class QStandardItem;
class QStandardItemModel;

class CollectionAttributePage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionAttributePage(QWidget *parent = nullptr);

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

private Q_SLOTS:
    void addAttribute();
    void delAttribute();
    void attributeChanged(QStandardItem *item);

private:
    Ui::CollectionAttributesPage ui;
    QStandardItemModel *mModel = nullptr;
    QSet<QString> mDeleted;
    QSet<QString> mChanged;
};

