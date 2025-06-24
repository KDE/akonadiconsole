/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "agentconfigmodel.h"
using namespace Qt::Literals::StringLiterals;

#include "akonadiconsole_debug.h"
#include <QDBusInterface>
#include <QMetaMethod>

AgentConfigModel::AgentConfigModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

AgentConfigModel::~AgentConfigModel()
{
    delete m_interface;
}

void AgentConfigModel::setAgentInstance(const Akonadi::AgentInstance &instance)
{
    beginResetModel();
    m_settings.clear();

    m_interface = new QDBusInterface(u"org.freedesktop.Akonadi.Agent.%1"_s.arg(instance.identifier()), u"/Settings"_s);
    if (!m_interface->isValid()) {
        qCritical() << "Unable to obtain KConfigXT D-Bus interface of agent" << instance.identifier();
        delete m_interface;
        return;
    }
    endResetModel();
    reload();
}

void AgentConfigModel::reload()
{
    beginResetModel();
    m_settings.clear();
    for (int i = 0; i < m_interface->metaObject()->methodCount(); ++i) {
        const QMetaMethod method = m_interface->metaObject()->method(i);
        if (QByteArray(method.typeName()).isEmpty()) { // returns void
            continue;
        }
        const QByteArray signature(method.methodSignature());
        if (signature.isEmpty()) {
            continue;
        }
        if (signature.startsWith("set") || !signature.contains("()")) { // setter or takes parameters // krazy:exclude=strings
            continue;
        }
        if (signature.startsWith("Introspect")) { // D-Bus stuff // krazy:exclude=strings
            continue;
        }
        const QString methodName = QString::fromLatin1(signature.left(signature.indexOf('(')));
        const QDBusMessage reply = m_interface->call(methodName);
        if (reply.arguments().count() != 1) {
            qCCritical(AKONADICONSOLE_LOG) << "call to method" << signature << "failed: " << reply.arguments() << reply.errorMessage();
            continue;
        }
        const QString settingName = methodName.at(0).toUpper() + methodName.mid(1);
        m_settings.append(qMakePair(settingName, reply.arguments().at(0)));
    }
    endResetModel();
}

int AgentConfigModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 3;
}

int AgentConfigModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) // flat model
        return 0;
    return m_settings.size();
}

QVariant AgentConfigModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_settings.size()) {
        return {};
    }
    QPair<QString, QVariant> setting = m_settings.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (index.column() == 0) {
            return setting.first;
        } else if (index.column() == 1) {
            if (setting.second.metaType().id() == QMetaType::QStringList) {
                return setting.second.toStringList().join(u", "_s);
            } else {
                return setting.second;
            }
        } else if (index.column() == 2) {
            return QLatin1StringView(setting.second.typeName());
        }
    }
    return {};
}

bool AgentConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() == 1 && role == Qt::EditRole && index.row() >= 0 && index.row() < m_settings.size()) {
        const QPair<QString, QVariant> setting = m_settings.at(index.row());
        if (setting.second != value) {
            m_interface->call(u"set%1"_s.arg(setting.first), value);
            reload();
        }
    }
    return QAbstractItemModel::setData(index, value, role);
}

QVariant AgentConfigModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return u"Setting"_s;
        } else if (section == 1) {
            return u"Value"_s;
        } else if (section == 2) {
            return u"Type"_s;
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags AgentConfigModel::flags(const QModelIndex &index) const
{
    if (index.column() == 1) {
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    }
    return QAbstractItemModel::flags(index);
}

void AgentConfigModel::writeConfig()
{
    m_interface->call(u"save"_s);
}

#include "moc_agentconfigmodel.cpp"
