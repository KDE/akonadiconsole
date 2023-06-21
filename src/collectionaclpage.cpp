/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionaclpage.h"

#include <Akonadi/Collection>
#include <KLocalizedString>

using namespace Akonadi;

CollectionAclPage::CollectionAclPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setPageTitle(i18n("ACL"));
    ui.setupUi(this);
}

void CollectionAclPage::load(const Collection &col)
{
    Collection::Rights rights = col.rights();
    ui.changeItem->setChecked(rights & Collection::CanChangeItem);
    ui.createItem->setChecked(rights & Collection::CanCreateItem);
    ui.deleteItem->setChecked(rights & Collection::CanDeleteItem);
    ui.linkItem->setChecked(rights & Collection::CanLinkItem);
    ui.unlinkItem->setChecked(rights & Collection::CanUnlinkItem);
    ui.changeCollection->setChecked(rights & Collection::CanChangeCollection);
    ui.createCollection->setChecked(rights & Collection::CanCreateCollection);
    ui.deleteCollection->setChecked(rights & Collection::CanDeleteCollection);
}

void CollectionAclPage::save(Collection &col)
{
    Collection::Rights rights = Collection::ReadOnly;
    if (ui.changeItem->isChecked()) {
        rights |= Collection::CanChangeItem;
    }
    if (ui.createItem->isChecked()) {
        rights |= Collection::CanCreateItem;
    }
    if (ui.deleteItem->isChecked()) {
        rights |= Collection::CanDeleteItem;
    }
    if (ui.changeCollection->isChecked()) {
        rights |= Collection::CanChangeCollection;
    }
    if (ui.createCollection->isChecked()) {
        rights |= Collection::CanCreateCollection;
    }
    if (ui.deleteCollection->isChecked()) {
        rights |= Collection::CanDeleteCollection;
    }
    if (ui.linkItem->isChecked()) {
        rights |= Collection::CanLinkItem;
    }
    if (ui.unlinkItem->isChecked()) {
        rights |= Collection::CanUnlinkItem;
    }
    col.setRights(rights);
}

#include "moc_collectionaclpage.cpp"
