/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "agentwidget.h"

#include "agentconfigdialog.h"
#include "akonadiconsole_debug.h"
#include <AkonadiCore/AgentFilterProxyModel>
#include <AkonadiCore/agentinstancecreatejob.h>
#include <AkonadiCore/agentmanager.h>
#include <AkonadiWidgets/AgentConfigurationDialog>
#include <AkonadiWidgets/agentinstancewidget.h>
#include <AkonadiWidgets/agenttypedialog.h>
#include <AkonadiWidgets/controlgui.h>

#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardGuiItem>
#include <QIcon>

#include <KGuiItem>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDialogButtonBox>
#include <QMenu>
#include <QMetaMethod>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

class TextDialog : public QDialog
{
public:
    explicit TextDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mText = new QPlainTextEdit(this);
        mText->setReadOnly(true);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &TextDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &TextDialog::reject);

        mainLayout->addWidget(mText);
        mainLayout->addWidget(buttonBox);
        resize(QSize(400, 600));
    }

    void setText(const QString &text)
    {
        mText->setPlainText(text);
    }

private:
    QPlainTextEdit *mText = nullptr;
};

using namespace Akonadi;

AgentWidget::AgentWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.instanceWidget, &Akonadi::AgentInstanceWidget::doubleClicked, this, &AgentWidget::configureAgent);
    connect(ui.instanceWidget, &Akonadi::AgentInstanceWidget::currentChanged, this, &AgentWidget::currentChanged);
    connect(ui.instanceWidget, &Akonadi::AgentInstanceWidget::customContextMenuRequested, this, &AgentWidget::showContextMenu);

    connect(ui.instanceWidget->view()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AgentWidget::selectionChanged);
    connect(ui.instanceWidget->view()->selectionModel(), &QItemSelectionModel::currentChanged, this, &AgentWidget::selectionChanged);
    connect(ui.instanceWidget->view()->model(), &QAbstractItemModel::dataChanged, this, &AgentWidget::slotDataChanged);

    currentChanged();

    KGuiItem::assign(ui.addButton, KStandardGuiItem::add());
    connect(ui.addButton, &QPushButton::clicked, this, &AgentWidget::addAgent);

    KGuiItem::assign(ui.removeButton, KStandardGuiItem::remove());
    connect(ui.removeButton, &QPushButton::clicked, this, &AgentWidget::removeAgent);

    mConfigMenu = new QMenu(QStringLiteral("Configure"), this);
    mConfigMenu->addAction(QStringLiteral("Configure Natively..."), this, &AgentWidget::configureAgent);
    mConfigMenu->addAction(QStringLiteral("Configure Remotely..."), this, &AgentWidget::configureAgentRemote);
    mConfigMenu->setIcon(KStandardGuiItem::configure().icon());
    KGuiItem::assign(ui.configButton, KStandardGuiItem::configure());
    ui.configButton->setMenu(mConfigMenu);
    connect(ui.configButton, &QPushButton::clicked, this, &AgentWidget::configureAgent);

    mSyncMenu = new QMenu(QStringLiteral("Synchronize"), this);
    mSyncMenu->addAction(QStringLiteral("Synchronize All"), this, &AgentWidget::synchronizeAgent);
    mSyncMenu->addAction(QStringLiteral("Synchronize Collection Tree"), this, &AgentWidget::synchronizeTree);
    mSyncMenu->addAction(QStringLiteral("Synchronize Tags"), this, [this]() {
        const AgentInstance::List list = ui.instanceWidget->selectedAgentInstances();
        for (auto agent : list) {
            agent.synchronizeTags();
        }
    });
    mSyncMenu->addAction(QStringLiteral("Synchronize Relations"), this, [this]() {
        const auto list = ui.instanceWidget->selectedAgentInstances();
        for (auto agent : list) {
            agent.synchronizeRelations();
        }
    });
    mSyncMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    ui.syncButton->setMenu(mSyncMenu);
    ui.syncButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    connect(ui.syncButton, &QPushButton::clicked, this, &AgentWidget::synchronizeAgent);

    ui.abortButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-cancel")));
    connect(ui.abortButton, &QPushButton::clicked, this, &AgentWidget::abortAgent);
    ui.restartButton->setIcon(QIcon::fromTheme(QStringLiteral("system-reboot"))); // FIXME: Is using system-reboot icon here a good idea?
    connect(ui.restartButton, &QPushButton::clicked, this, &AgentWidget::restartAgent);

    ui.instanceWidget->agentFilterProxyModel()->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(ui.mFilterAccount, &QLineEdit::textChanged, this, &AgentWidget::slotSearchAgentType);
    ui.mFilterAccount->installEventFilter(this);
    ControlGui::widgetNeedsAkonadi(this);
}

void AgentWidget::slotSearchAgentType(const QString &str)
{
    ui.instanceWidget->agentFilterProxyModel()->setFilterRegularExpression(str);
}

bool AgentWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && obj == ui.mFilterAccount) {
        auto key = static_cast<QKeyEvent *>(event);
        if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return)) {
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AgentWidget::addAgent()
{
    QPointer<Akonadi::AgentTypeDialog> dlg = new Akonadi::AgentTypeDialog(this);
    if (dlg->exec()) {
        const AgentType agentType = dlg->agentType();

        if (agentType.isValid()) {
            auto job = new AgentInstanceCreateJob(agentType, this);
            job->configure(this);
            job->start(); // TODO: check result
        }
    }
    delete dlg;
}

void AgentWidget::selectionChanged()
{
    const bool multiSelection = ui.instanceWidget->view()->selectionModel()->selectedRows().size() > 1;
    // Only agent removal, sync and restart is possible when multiple items are selected.
    ui.configButton->setDisabled(multiSelection);

    // Restarting an agent is not possible if it's in Running status... (see AgentProcessInstance::restartWhenIdle)
    AgentInstance agent = ui.instanceWidget->currentAgentInstance();
    ui.restartButton->setEnabled(agent.isValid() && agent.status() != 1);
}

void AgentWidget::slotDataChanged(const QModelIndex &topLeft, const QModelIndex & /*bottomRight*/)
{
    QList<QModelIndex> selectedRows = ui.instanceWidget->view()->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        selectedRows.append(ui.instanceWidget->view()->selectionModel()->currentIndex());
    }
    QList<int> rows;
    for (const QModelIndex &index : std::as_const(selectedRows)) {
        rows.append(index.row());
    }
    std::sort(rows.begin(), rows.end());
    // Assume topLeft.row == bottomRight.row
    if (topLeft.row() >= rows.first() && topLeft.row() <= rows.last()) {
        selectionChanged(); // depends on status
        currentChanged();
    }
}

void AgentWidget::removeAgent()
{
    const AgentInstance::List list = ui.instanceWidget->selectedAgentInstances();
    if (!list.isEmpty()) {
        if (KMessageBox::questionYesNo(
                this,
                i18np("Do you really want to delete the selected agent instance?", "Do you really want to delete these %1 agent instances?", list.size()),
                list.size() == 1 ? QStringLiteral("Agent Deletion") : QStringLiteral("Multiple Agent Deletion"),
                KStandardGuiItem::del(),
                KStandardGuiItem::cancel(),
                QString(),
                KMessageBox::Dangerous)
            == KMessageBox::Yes) {
            for (const AgentInstance &agent : list) {
                AgentManager::self()->removeInstance(agent);
            }
        }
    }
}

void AgentWidget::configureAgent()
{
    AgentInstance agent = ui.instanceWidget->currentAgentInstance();
    if (agent.isValid()) {
        QPointer<AgentConfigurationDialog> dlg = new AgentConfigurationDialog(agent, this);
        dlg->exec();
        delete dlg;
    }
}

void AgentWidget::configureAgentRemote()
{
    AgentInstance agent = ui.instanceWidget->currentAgentInstance();
    if (agent.isValid()) {
        QPointer<AgentConfigDialog> dlg = new AgentConfigDialog(this);
        dlg->setAgentInstance(agent);
        dlg->exec();
        delete dlg;
    }
}

void AgentWidget::synchronizeAgent()
{
    const AgentInstance::List list = ui.instanceWidget->selectedAgentInstances();
    if (!list.isEmpty()) {
        for (AgentInstance agent : list) {
            agent.synchronize();
        }
    }
}

void AgentWidget::toggleOnline()
{
    AgentInstance agent = ui.instanceWidget->currentAgentInstance();
    if (agent.isValid()) {
        agent.setIsOnline(!agent.isOnline());
    }
}

void AgentWidget::showTaskList()
{
    AgentInstance agent = ui.instanceWidget->currentAgentInstance();
    if (!agent.isValid()) {
        return;
    }

    QDBusInterface iface(QStringLiteral("org.freedesktop.Akonadi.Agent.%1").arg(agent.identifier()), QStringLiteral("/Debug"), QString());

    QDBusReply<QString> reply = iface.call(QStringLiteral("dumpToString"));
    QString txt;
    if (reply.isValid()) {
        txt = reply.value();
    } else {
        txt = reply.error().message();
    }

    QPointer<TextDialog> dlg = new TextDialog(this);
    dlg->setWindowTitle(QStringLiteral("Resource Task List"));
    dlg->setText(txt);
    dlg->exec();
    delete dlg;
}

void AgentWidget::showChangeNotifications()
{
    AgentInstance agent = ui.instanceWidget->currentAgentInstance();
    if (!agent.isValid()) {
        return;
    }

    QDBusInterface iface(QStringLiteral("org.freedesktop.Akonadi.Agent.%1").arg(agent.identifier()), QStringLiteral("/Debug"), QString());

    QDBusReply<QString> reply = iface.call(QStringLiteral("dumpNotificationListToString"));
    QString txt;
    if (reply.isValid()) {
        txt = reply.value();
    } else {
        txt = reply.error().message();
    }

    QPointer<TextDialog> dlg = new TextDialog(this);
    dlg->setWindowTitle(QStringLiteral("Change Notification Log"));
    dlg->setText(txt);

    dlg->exec();
    delete dlg;
}

void AgentWidget::synchronizeTree()
{
    const AgentInstance::List list = ui.instanceWidget->selectedAgentInstances();
    for (AgentInstance agent : list) {
        agent.synchronizeCollectionTree();
    }
}

void AgentWidget::abortAgent()
{
    const AgentInstance::List list = ui.instanceWidget->selectedAgentInstances();
    for (const AgentInstance &agent : list) {
        agent.abortCurrentTask();
    }
}

void AgentWidget::restartAgent()
{
    AgentInstance agent = ui.instanceWidget->currentAgentInstance();
    if (agent.isValid()) {
        agent.restart();
    }
}

void AgentWidget::slotCloneAgent()
{
    mCloneSource = ui.instanceWidget->currentAgentInstance();
    if (!mCloneSource.isValid()) {
        return;
    }
    const AgentType agentType = mCloneSource.type();
    if (agentType.isValid()) {
        auto job = new AgentInstanceCreateJob(agentType, this);
        connect(job, &KJob::result, this, &AgentWidget::cloneAgent);
        job->start();
    } else {
        qCWarning(AKONADICONSOLE_LOG) << "WTF?";
    }
}

void AgentWidget::cloneAgent(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(this, QStringLiteral("Cloning agent failed: %1.").arg(job->errorText()));
        return;
    }

    AgentInstance cloneTarget = static_cast<AgentInstanceCreateJob *>(job)->instance();
    Q_ASSERT(cloneTarget.isValid());
    Q_ASSERT(mCloneSource.isValid());

    QDBusInterface sourceIface(QStringLiteral("org.freedesktop.Akonadi.Agent.%1").arg(mCloneSource.identifier()), QStringLiteral("/Settings"));
    if (!sourceIface.isValid()) {
        qCritical() << "Unable to obtain KConfigXT D-Bus interface of source agent" << mCloneSource.identifier();
        return;
    }

    QDBusInterface targetIface(QStringLiteral("org.freedesktop.Akonadi.Agent.%1").arg(cloneTarget.identifier()), QStringLiteral("/Settings"));
    if (!targetIface.isValid()) {
        qCritical() << "Unable to obtain KConfigXT D-Bus interface of target agent" << cloneTarget.identifier();
        return;
    }

    cloneTarget.setName(mCloneSource.name() + QStringLiteral(" (Clone)"));

    // iterate over all getter methods in the source interface and call the
    // corresponding setter in the target interface
    for (int i = 0; i < sourceIface.metaObject()->methodCount(); ++i) {
        const QMetaMethod method = sourceIface.metaObject()->method(i);
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
        const QString methodName = QLatin1String(signature.left(signature.indexOf('(')));
        const QDBusMessage reply = sourceIface.call(methodName);
        if (reply.arguments().count() != 1) {
            qCritical() << "call to method" << signature << "failed: " << reply.arguments() << reply.errorMessage();
            continue;
        }
        const QString setterName = QStringLiteral("set") + methodName.at(0).toUpper() + methodName.mid(1);
        targetIface.call(setterName, reply.arguments().at(0));
    }

    cloneTarget.reconfigure();
}

void AgentWidget::currentChanged()
{
    AgentInstance instance = ui.instanceWidget->currentAgentInstance();
    ui.removeButton->setEnabled(instance.isValid());
    ui.configButton->setEnabled(instance.isValid());
    ui.syncButton->setEnabled(instance.isValid());
    ui.restartButton->setEnabled(instance.isValid());

    if (instance.isValid()) {
        ui.identifierLabel->setText(instance.identifier());
        ui.typeLabel->setText(instance.type().name());
        QString onlineStatus = (instance.isOnline() ? QStringLiteral("Online") : QStringLiteral("Offline"));
        QString agentStatus;
        switch (instance.status()) {
        case AgentInstance::Idle:
            agentStatus = i18nc("agent is in an idle state", "Idle");
            break;
        case AgentInstance::Running:
            agentStatus = i18nc("agent is running", "Running (%1%)", instance.progress());
            break;
        case AgentInstance::Broken:
            agentStatus = i18nc("agent is broken somehow", "Broken");
            break;
        case AgentInstance::NotConfigured:
            agentStatus = i18nc("agent is not yet configured", "Not Configured");
            break;
        }
        ui.statusLabel->setText(i18nc("Two statuses, for example \"Online, Running (66%)\" or \"Offline, Broken\"", "%1, %2", onlineStatus, agentStatus));
        ui.statusMessageLabel->setText(instance.statusMessage());
        ui.capabilitiesLabel->setText(instance.type().capabilities().join(QLatin1String(", ")));
        ui.mimeTypeLabel->setText(instance.type().mimeTypes().join(QLatin1String(", ")));
    } else {
        ui.identifierLabel->setText(QString());
        ui.typeLabel->setText(QString());
        ui.statusLabel->setText(QString());
        ui.capabilitiesLabel->setText(QString());
        ui.mimeTypeLabel->setText(QString());
    }
}

void AgentWidget::showContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-add")), QStringLiteral("Add Agent..."), this, &AgentWidget::addAgent);
    menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), QStringLiteral("Clone Agent"), this, &AgentWidget::slotCloneAgent);
    menu.addSeparator();
    menu.addMenu(mSyncMenu);
    menu.addAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), QStringLiteral("Abort Activity"), this, &AgentWidget::abortAgent);
    menu.addAction(QIcon::fromTheme(QStringLiteral("system-reboot")),
                   QStringLiteral("Restart Agent"),
                   this,
                   &AgentWidget::restartAgent); // FIXME: Is using system-reboot icon here a good idea?
    menu.addAction(QIcon::fromTheme(QStringLiteral("network-disconnect")), QStringLiteral("Toggle Online/Offline"), this, &AgentWidget::toggleOnline);
    menu.addAction(QStringLiteral("Show task list"), this, &AgentWidget::showTaskList);
    menu.addAction(QStringLiteral("Show change-notification log"), this, &AgentWidget::showChangeNotifications);
    menu.addMenu(mConfigMenu);
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), QStringLiteral("Remove Agent"), this, &AgentWidget::removeAgent);
    menu.exec(ui.instanceWidget->mapToGlobal(pos));
}

void AgentWidget::resizeEvent(QResizeEvent *event)
{
    ui.detailsBox->setVisible(event->size().height() > 400);
}
