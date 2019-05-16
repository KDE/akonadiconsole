/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "logging.h"
#include "loggeradaptor.h"
#include "loggingmodel.h"
#include "loggingfiltermodel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QDBusConnection>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>

#include <KCheckComboBox>

#include <KSharedConfig>
#include <KConfigGroup>

Q_DECLARE_METATYPE(LoggingModel::Message)

#define DBUS_PATH QStringLiteral("/logger")
#define DBUS_INTERFACE QStringLiteral("org.kde.akonadiconsole.logger")

using namespace KPIM;

Logging::Logging(QWidget *parent)
    : QWidget(parent)
{
    auto l = new QVBoxLayout(this);
    setLayout(l);

    mEnabledCheckbox = new QCheckBox(QStringLiteral("Enable"));
    connect(mEnabledCheckbox, &QCheckBox::toggled,
            this, [this](bool toggled) {
                Q_EMIT enabledChanged(toggled);
            });
    l->addWidget(mEnabledCheckbox);

    auto h = new QHBoxLayout;
    l->addLayout(h);

    h->addWidget(new QLabel(QStringLiteral("Programs:")));
    h->addWidget(mAppFilter = new KCheckComboBox());
    h->setStretchFactor(mAppFilter, 2);
    h->addWidget(new QLabel(QStringLiteral("Types:")));
    h->addWidget(mTypeFilter = new KCheckComboBox());
    h->setStretchFactor(mTypeFilter, 2);
    mTypeFilter->addItem(QStringLiteral("Debug"), QtDebugMsg);
    mTypeFilter->addItem(QStringLiteral("Info"), QtInfoMsg);
    mTypeFilter->addItem(QStringLiteral("Warning"), QtWarningMsg);
    mTypeFilter->addItem(QStringLiteral("Critical"), QtCriticalMsg);
    mTypeFilter->addItem(QStringLiteral("Fatal"), QtFatalMsg);
    h->addWidget(new QLabel(QStringLiteral("Categories:")));
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
    mModel->setAppFilterModel(qobject_cast<QStandardItemModel*>(mAppFilter->model()));
    mModel->setCategoryFilterModel(qobject_cast<QStandardItemModel*>(mCategoryFilter->model()));

    h = new QHBoxLayout;
    l->addLayout(h);

    auto btn = new QPushButton(QStringLiteral("Save to File..."));
    connect(btn, &QPushButton::clicked, this, &Logging::saveToFile);
    h->addWidget(btn);
    h->addStretch(1);

    new LoggerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(DBUS_PATH, this, QDBusConnection::ExportAdaptors);

    KConfigGroup config(KSharedConfig::openConfig(), "Logging");
    mView->header()->restoreState(config.readEntry<QByteArray>("view", QByteArray()));
}

Logging::~Logging()
{
    KConfigGroup config(KSharedConfig::openConfig(), "Logging");
    config.writeEntry("view", mView->header()->saveState());
}


bool Logging::enabled() const
{
    return mEnabledCheckbox->isChecked();
}

void Logging::message(qint64 timestamp, const QString &app, qint64 pid, int type, const QString &category,
                      const QString &file, const QString &function, int line,
                      int /*version*/, const QString &msg)
{
    mModel->addMessage(timestamp, app, pid, static_cast<QtMsgType>(type), category, file, function, line, msg);
}

void Logging::saveToFile()
{
    const auto filename = QFileDialog::getSaveFileName(this, QStringLiteral("Save to File..."));
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Failed to open file: %1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    for (int row = 0, cnt = mModel->rowCount(); row < cnt; ++row) {
        const auto msg = mModel->data(mModel->index(row, 0), LoggingModel::MessageRole).value<LoggingModel::Message>();
        stream << "[" << QDateTime::fromMSecsSinceEpoch(msg.timestamp, Qt::UTC).toString(Qt::ISODateWithMs) << "] "
               << msg.app << " ";
        if (!msg.category.isEmpty()) {
            stream << msg.category << " ";
        }
        if (!msg.function.isEmpty()) {
               stream << msg.function << ": ";
        }
        stream << msg.message << "\n";
    }
}
