/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "agentconfigdialog.h"
#include "agentconfigmodel.h"
#include <KGuiItem>
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

AgentConfigDialog::AgentConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_model(new AgentConfigModel(this))
{
    auto mainWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    ui.setupUi(mainWidget);
    ui.propertyView->setModel(m_model);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Apply, this);
    auto user1Button = new QPushButton;
    buttonBox->addButton(user1Button, QDialogButtonBox::ActionRole);
    auto user2Button = new QPushButton;
    buttonBox->addButton(user2Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AgentConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AgentConfigDialog::reject);
    mainLayout->addWidget(buttonBox);
    KGuiItem::assign(user1Button, KGuiItem(i18n("Save Configuration")));
    KGuiItem::assign(user2Button, KGuiItem(i18n("Refresh")));
    buttonBox->button(QDialogButtonBox::Apply)->setText(i18n("Apply Configuration"));

    setWindowTitle(i18n("Agent Configuration"));

    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &AgentConfigDialog::reconfigure);
    connect(user1Button, &QPushButton::clicked, m_model, &AgentConfigModel::writeConfig);
    connect(user2Button, &QPushButton::clicked, m_model, &AgentConfigModel::reload);
}

void AgentConfigDialog::setAgentInstance(const Akonadi::AgentInstance &instance)
{
    m_instance = instance;
    m_model->setAgentInstance(instance);
}

void AgentConfigDialog::reconfigure()
{
    m_instance.reconfigure();
}

#include "moc_agentconfigdialog.cpp"
