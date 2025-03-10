/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "agentwidget.h"
using namespace Qt::Literals::StringLiterals;

#include "agentconfigdialog.h"
#include "akonadiconsole_debug.h"
#include <Akonadi/AgentConfigurationDialog>
#include <Akonadi/AgentFilterProxyModel>
#include <Akonadi/AgentInstanceCreateJob>
#include <Akonadi/AgentInstanceFilterProxyModel>
#include <Akonadi/AgentInstanceWidget>
#include <Akonadi/AgentManager>
#include <Akonadi/AgentTypeDialog>
#include <Akonadi/ControlGui>

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
        , mText(new QPlainTextEdit(this))
    {
        auto mainLayout = new QVBoxLayout(this);
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
    QPlainTextEdit *const mText;
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

    mConfigMenu = new QMenu(i18n("Configure"), this);
    mConfigMenu->addAction(i18n("Configure Natively..."), this, &AgentWidget::configureAgent);
    mConfigMenu->addAction(i18n("Configure Remotely..."), this, &AgentWidget::configureAgentRemote);
    mConfigMenu->setIcon(KStandardGuiItem::configure().icon());
    KGuiItem::assign(ui.configButton, KStandardGuiItem::configure());
    ui.configButton->setMenu(mConfigMenu);
    connect(ui.configButton, &QPushButton::clicked, this, &AgentWidget::configureAgent);

    mSyncMenu = new QMenu(i18n("Synchronize"), this);
    mSyncMenu->addAction(i18n("Synchronize All"), this, &AgentWidget::synchronizeAgent);
    mSyncMenu->addAction(i18n("Synchronize Collection Tree"), this, &AgentWidget::synchronizeTree);
    mSyncMenu->addAction(i18n("Synchronize Tags"), this, [this]() {
        const AgentInstance::List list = ui.instanceWidget->selectedAgentInstances();
        for (auto agent : list) {
            agent.synchronizeTags();
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

    ui.instanceWidget->agentInstanceFilterProxyModel()->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(ui.mFilterAccount, &QLineEdit::textChanged, this, &AgentWidget::slotSearchAgentType);
    ui.mFilterAccount->installEventFilter(this);
    ControlGui::widgetNeedsAkonadi(this);
}

void AgentWidget::slotSearchAgentType(const QString &str)
{
    ui.instanceWidget->agentInstanceFilterProxyModel()->setFilterRegularExpression(str);
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
            connect(job, &Akonadi::AgentInstanceCreateJob::result, this, [this, job](KJob *) {
                if (job->error()) {
                    qCWarning(AKONADICONSOLE_LOG) << job->errorText();
                    return;
                }
                auto configureDialog = new Akonadi::AgentConfigurationDialog(job->instance(), this);
                configureDialog->setAttribute(Qt::WA_DeleteOnClose);
                connect(configureDialog, &QDialog::rejected, this, [instance = job->instance()] {
                    Akonadi::AgentManager::self()->removeInstance(instance);
                });
                configureDialog->show();
            });

            job->start();
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
        if (KMessageBox::questionTwoActions(
                this,
                i18np("Do you really want to delete the selected agent instance?", "Do you really want to delete these %1 agent instances?", list.size()),
                list.size() == 1 ? i18n("Agent Deletion") : i18n("Multiple Agent Deletion"),
                KStandardGuiItem::del(),
                KStandardGuiItem::cancel(),
                QString(),
                KMessageBox::Dangerous)
            == KMessageBox::ButtonCode::PrimaryAction) {
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
    dlg->setWindowTitle(i18nc("@title:window", "Resource Task List"));
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
    dlg->setWindowTitle(i18nc("@title:window", "Change Notification Log"));
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
        KMessageBox::error(this, i18n("Cloning agent failed: %1.", job->errorText()));
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
        const QString methodName = QLatin1StringView(signature.left(signature.indexOf('(')));
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
        QString onlineStatus = (instance.isOnline() ? i18n("Online") : i18n("Offline"));
        QString agentStatus;
        switch (instance.status()) {
        case AgentInstance::Idle:
            agentStatus = i18n("Idle");
            break;
        case AgentInstance::Running:
            agentStatus = i18n("Running (%1%)", instance.progress());
            break;
        case AgentInstance::Broken:
            agentStatus = i18n("Broken");
            break;
        case AgentInstance::NotConfigured:
            agentStatus = i18n("Not Configured");
            break;
        }
        ui.statusLabel->setText(i18nc("Two statuses, for example \"Online, Running (66%)\" or \"Offline, Broken\"", "%1, %2", onlineStatus, agentStatus));
        ui.statusMessageLabel->setText(instance.statusMessage());
        ui.capabilitiesLabel->setText(instance.type().capabilities().join(", "_L1));
        ui.mimeTypeLabel->setText(instance.type().mimeTypes().join(", "_L1));
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
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add Agent..."), this, &AgentWidget::addAgent);
    menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Clone Agent"), this, &AgentWidget::slotCloneAgent);
    menu.addSeparator();
    menu.addMenu(mSyncMenu);
    menu.addAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), i18n("Abort Activity"), this, &AgentWidget::abortAgent);
    menu.addAction(QIcon::fromTheme(QStringLiteral("system-reboot")),
                   i18n("Restart Agent"),
                   this,
                   &AgentWidget::restartAgent); // FIXME: Is using system-reboot icon here a good idea?
    menu.addAction(QIcon::fromTheme(QStringLiteral("network-disconnect")), i18n("Toggle Online/Offline"), this, &AgentWidget::toggleOnline);
    menu.addAction(i18n("Show task list"), this, &AgentWidget::showTaskList);
    menu.addAction(i18n("Show change-notification log"), this, &AgentWidget::showChangeNotifications);
    menu.addMenu(mConfigMenu);
    menu.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove Agent"), this, &AgentWidget::removeAgent);
    menu.exec(ui.instanceWidget->mapToGlobal(pos));
}

void AgentWidget::resizeEvent(QResizeEvent *event)
{
    ui.detailsBox->setVisible(event->size().height() > 400);
}

#include "moc_agentwidget.cpp"
