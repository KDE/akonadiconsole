/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "browserwidget.h"

#include <QWidget>

class KXmlGuiWindow;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(KXmlGuiWindow *parent = nullptr);
    ~MainWidget() override;

private Q_SLOTS:
    void startServer();
    void stopServer();
    void restartServer();

private:
    BrowserWidget *mBrowser = nullptr;
};
