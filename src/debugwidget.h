/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_DEBUGWIDGET_H
#define AKONADICONSOLE_DEBUGWIDGET_H

#include "debuginterface.h"

#include <QHash>
#include <QWidget>

class KTextEdit;

class ConnectionPage;

class DebugWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DebugWidget(QWidget *parent = nullptr);

private Q_SLOTS:
    void signalEmitted(const QString &, const QString &);
    void warningEmitted(const QString &, const QString &);
    void errorEmitted(const QString &, const QString &);

    void enableDebugger(bool enable);

    void saveRichText();
    void saveEverythingRichText();

private:
    KTextEdit *mGeneralView = nullptr;
    ConnectionPage *mConnectionPage = nullptr;
    org::freedesktop::Akonadi::DebugInterface *mDebugInterface = nullptr;
};

#endif
