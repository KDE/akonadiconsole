/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
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
