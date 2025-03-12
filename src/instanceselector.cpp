/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "instanceselector.h"
#include "ui_instanceselector.h"

#include "akonadiconsole_debug.h"
#include <KLocalizedString>
#include <QIcon>
#include <private/dbus_p.h>
#include <private/instance_p.h>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

InstanceSelector::InstanceSelector(const QString &remoteHost, QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , ui(new Ui::InstanceSelector)
    , m_remoteHost(remoteHost)
{
    auto mainWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    ui->setupUi(mainWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Close, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &InstanceSelector::slotAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &InstanceSelector::slotReject);
    mainLayout->addWidget(buttonBox);
    okButton->setIcon(QIcon::fromTheme(QStringLiteral("network-connect")));
    okButton->setText(i18n("Connect"));

    const QStringList insts = instances();
    qCDebug(AKONADICONSOLE_LOG) << "Found running Akonadi instances:" << insts;
    if (insts.size() <= 1) {
        m_instance = QString::fromUtf8(qgetenv("AKONADI_INSTANCE"));
        if (insts.size() == 1 && m_instance.isEmpty()) {
            m_instance = insts.first();
        }
        slotAccept();
    } else {
        auto model = new QStandardItemModel(this);
        for (const QString &inst : insts) {
            auto item = new QStandardItem;
            item->setText(inst.isEmpty() ? QStringLiteral("<global>") : inst);
            item->setData(inst, Qt::UserRole);
            model->appendRow(item);
        }
        connect(ui->listView, &QAbstractItemView::doubleClicked, this, &InstanceSelector::slotAccept);
        ui->listView->setModel(model);
        show();
    }
}

InstanceSelector::~InstanceSelector()
{
    delete mWindow;
}

void InstanceSelector::slotAccept()
{
    if (ui->listView->model()) { // there was something to select
        const QModelIndexList selection = ui->listView->selectionModel()->selectedRows();
        if (selection.size() != 1) {
            return;
        }
        m_instance = selection.first().data(Qt::UserRole).toString();
    }
    QDialog::accept();

    qputenv("AKONADI_INSTANCE", m_instance.toUtf8());
    Akonadi::Instance::setIdentifier(m_instance);
    auto mWindow = new MainWindow;
    if (!m_remoteHost.isEmpty()) {
        mWindow->setWindowTitle(i18nc("@title:window", "Remote Akonadi Console (%1)", m_remoteHost));
    } else if (!m_instance.isEmpty()) {
        mWindow->setWindowTitle(i18nc("@title:window", "Akonadi Console (Instance: %1)", m_instance));
    }
    mWindow->show();
}

void InstanceSelector::slotReject()
{
    QDialog::reject();
    QApplication::quit();
}

QStringList InstanceSelector::instances()
{
    const QString currentInstance = Akonadi::Instance::identifier();
    Akonadi::Instance::setIdentifier(QString());
    const QString lockService = Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock);
    Akonadi::Instance::setIdentifier(currentInstance);

    const QStringList allServices = QDBusConnection::sessionBus().interface()->registeredServiceNames();
    QStringList insts;
    for (const QString &service : allServices) {
        if (!service.startsWith(lockService)) {
            continue;
        }
        insts.push_back(service.mid(lockService.length() + 1));
    }
    return insts;
}

#include "moc_instanceselector.cpp"
