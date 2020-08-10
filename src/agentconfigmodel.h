/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_AGENTCONFIGMODEL_H
#define AKONADICONSOLE_AGENTCONFIGMODEL_H

#include <AkonadiCore/AgentInstance>

#include <QAbstractItemModel>
#include <QVector>

class QDBusInterface;

class AgentConfigModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AgentConfigModel(QObject *parent = nullptr);
    ~AgentConfigModel();
    void setAgentInstance(const Akonadi::AgentInstance &instance);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

public Q_SLOTS:
    void reload();
    void writeConfig();

private:
    QVector<QPair<QString, QVariant> > m_settings;
    QDBusInterface *m_interface = nullptr;
};

#endif
