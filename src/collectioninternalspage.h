/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_COLLECTIONINTERNALSPAGE_H
#define AKONADICONSOLE_COLLECTIONINTERNALSPAGE_H

#include "ui_collectioninternalspage.h"

#include <AkonadiWidgets/collectionpropertiespage.h>

class CollectionInternalsPage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionInternalsPage(QWidget *parent = nullptr);

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

private:
    Ui::CollectionInternalsPage ui;
};

#endif
