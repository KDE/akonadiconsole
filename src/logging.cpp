/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "logging.h"
#include "loggeradaptor.h"
#include "loggingfiltermodel.h"
#include "loggingmodel.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QDBusConnection>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>

#include <Libkdepim/KCheckComboBox>

#include <KConfigGroup>
#include <KSharedConfig>

#ifndef COMPILE_WITH_UNITY_CMAKE_SUPPORT
Q_DECLARE_METATYPE(LoggingModel::Message)
#endif
#define DBUS_PATH QStringLiteral("/logger")
#define DBUS_INTERFACE QStringLiteral("org.kde.akonadiconsole.logger")

using namespace KPIM;

Logging::Logging(QWidget *parent)
    : QWidget(parent)
{
    auto l = new QVBoxLayout(this);
    setLayout(l);

    mEnabledCheckbox = new QCheckBox(i18n("Enable"), this);
    connect(mEnabledCheckbox, &QCheckBox::toggled, this, [this](bool toggled) {
        Q_EMIT enabledChanged(toggled);
    });
    l->addWidget(mEnabledCheckbox);

    auto h = new QHBoxLayout;
    l->addLayout(h);

    h->addWidget(new QLabel(i18n("Programs:"), this));
    h->addWidget(mAppFilter = new KCheckComboBox());
    h->setStretchFactor(mAppFilter, 2);
    h->addWidget(new QLabel(i18n("Types:"), this));
    h->addWidget(mTypeFilter = new KCheckComboBox());
    h->setStretchFactor(mTypeFilter, 2);
    mTypeFilter->addItem(i18n("Debug"), QtDebugMsg);
    mTypeFilter->addItem(i18n("Info"), QtInfoMsg);
    mTypeFilter->addItem(i18n("Warning"), QtWarningMsg);
    mTypeFilter->addItem(i18n("Critical"), QtCriticalMsg);
    mTypeFilter->addItem(i18n("Fatal"), QtFatalMsg);
    h->addWidget(new QLabel(i18n("Categories:")));
    h->addWidget(mCategoryFilter = new KCheckComboBox());
    h->setStretchFactor(mCategoryFilter, 2);

    mModel = new LoggingModel(this);

    auto filterModel = new LoggingFilterModel(this);
    filterModel->setAppFilter(mAppFilter);
    filterModel->setCategoryFilter(mCategoryFilter);
    filterModel->setTypeFilter(mTypeFilter);
    filterModel->setSourceModel(mModel);

    for (int i = 0; i < mTypeFilter->count(); ++i) {
        mTypeFilter->setItemCheckState(i, Qt::Checked);
    }

    mView = new QTreeView;
    mView->setRootIsDecorated(false);
    l->addWidget(mView);
    mView->setModel(filterModel);
    mModel->setAppFilterModel(qobject_cast<QStandardItemModel *>(mAppFilter->model()));
    mModel->setCategoryFilterModel(qobject_cast<QStandardItemModel *>(mCategoryFilter->model()));

    h = new QHBoxLayout;
    l->addLayout(h);

    auto btn = new QPushButton(i18n("Save to File..."), this);
    connect(btn, &QPushButton::clicked, this, &Logging::saveToFile);
    h->addWidget(btn);
    h->addStretch(1);

    new LoggerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(DBUS_PATH, this, QDBusConnection::ExportAdaptors);

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Logging"));
    mView->header()->restoreState(config.readEntry<QByteArray>("view", QByteArray()));
}

Logging::~Logging()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Logging"));
    config.writeEntry("view", mView->header()->saveState());
}

bool Logging::enabled() const
{
    return mEnabledCheckbox->isChecked();
}

void Logging::message(qint64 timestamp,
                      const QString &app,
                      qint64 pid,
                      int type,
                      const QString &category,
                      const QString &file,
                      const QString &function,
                      int line,
                      int /*version*/,
                      const QString &msg)
{
    mModel->addMessage(timestamp, app, pid, static_cast<QtMsgType>(type), category, file, function, line, msg);
}

void Logging::saveToFile()
{
    const auto filename = QFileDialog::getSaveFileName(this, i18n("Save to File..."));
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, i18n("Error"), i18n("Failed to open file: %1", file.errorString()));
        return;
    }

    QTextStream stream(&file);
    for (int row = 0, cnt = mModel->rowCount(); row < cnt; ++row) {
        const auto msg = mModel->data(mModel->index(row, 0), LoggingModel::MessageRole).value<LoggingModel::Message>();
        stream << "[" << QDateTime::fromMSecsSinceEpoch(msg.timestamp, Qt::UTC).toString(Qt::ISODateWithMs) << "] " << msg.app << " ";
        if (!msg.category.isEmpty()) {
            stream << msg.category << " ";
        }
        if (!msg.function.isEmpty()) {
            stream << msg.function << ": ";
        }
        stream << msg.message << "\n";
    }
}

#include "moc_logging.cpp"
