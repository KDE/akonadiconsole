/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectioninternalspage.h"

#include <Akonadi/Collection>
using namespace Akonadi;

CollectionInternalsPage::CollectionInternalsPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setPageTitle(QStringLiteral("Internals"));
    ui.setupUi(this);
}

void CollectionInternalsPage::load(const Akonadi::Collection &col)
{
    ui.idLabel->setText(QString::number(col.id()));
    ui.ridEdit->setText(col.remoteId());
    ui.rrevEdit->setText(col.remoteRevision());
    ui.resourceLabel->setText(col.resource());
    ui.contentTypes->setItems(col.contentMimeTypes());
    ui.virtCheck->setChecked(col.isVirtual());
}

void CollectionInternalsPage::save(Akonadi::Collection &col)
{
    col.setRemoteId(ui.ridEdit->text());
    col.setRemoteRevision(ui.rrevEdit->text());
    col.setContentMimeTypes(ui.contentTypes->items());
    col.setVirtual(ui.virtCheck->isChecked());
}
