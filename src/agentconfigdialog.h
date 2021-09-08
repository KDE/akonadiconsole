/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "ui_agentconfigdialog.h"
#include <Akonadi/AgentInstance>
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
    AgentConfigModel *const m_model;
    Akonadi::AgentInstance m_instance;
};

