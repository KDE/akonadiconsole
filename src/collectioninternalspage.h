/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ui_collectioninternalspage.h"

#include <Akonadi/CollectionPropertiesPage>

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

