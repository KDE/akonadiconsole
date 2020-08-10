/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_AGENTCONFIGDIALOG_H
#define AKONADICONSOLE_AGENTCONFIGDIALOG_H

#include "ui_agentconfigdialog.h"
#include <AkonadiCore/AgentInstance>
#include <QDialog>

class AgentConfigModel;

class AgentConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AgentConfigDialog(QWidget *parent = nullptr);
    void setAgentInstance(const Akonadi::AgentInstance &instance);

private Q_SLOTS:
    void reconfigure();

private:
    Ui::AgentConfigDialog ui;
    AgentConfigModel *m_model = nullptr;
    Akonadi::AgentInstance m_instance;
};

#endif
