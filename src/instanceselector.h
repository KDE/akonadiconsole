/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "libakonadiconsole_export.h"
#include "mainwindow.h"

#include <QDialog>

namespace Ui
{
class InstanceSelector;
}

/** Check if there are multiple instances of Akonadi running, and if so present
 *  a list to select the one to connect to.
 */
class LIBAKONADICONSOLE_EXPORT InstanceSelector : public QDialog
{
    Q_OBJECT
public:
    explicit InstanceSelector(const QString &remoteHost, QWidget *parent = nullptr, Qt::WindowFlags flags = {});
    ~InstanceSelector() override;

private:
    static QStringList instances();

private:
    void slotAccept();
    void slotReject();
    QScopedPointer<Ui::InstanceSelector> ui;
    const QString m_remoteHost;
    QString m_instance;
    MainWindow *mWindow = nullptr;
};
