/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "debuginterface.h"

#include <QWidget>

class KTextEdit;

class ConnectionPage;

class DebugWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DebugWidget(QWidget *parent = nullptr);

private:
    void signalEmitted(const QString &, const QString &);
    void warningEmitted(const QString &, const QString &);
    void errorEmitted(const QString &, const QString &);

    void enableDebugger(bool enable);

    void saveRichText();
    void saveEverythingRichText();
    KTextEdit *mGeneralView = nullptr;
    ConnectionPage *mConnectionPage = nullptr;
    org::freedesktop::Akonadi::DebugInterface *mDebugInterface = nullptr;
};
