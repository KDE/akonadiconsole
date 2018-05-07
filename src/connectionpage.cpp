/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "connectionpage.h"

#include <QHeaderView>
#include <QTableView>

#include <QVBoxLayout>
#include <QFontDatabase>

#include "tracernotificationinterface.h"
#include <QStandardItemModel>

ConnectionPage::ConnectionPage(const QString &identifier, QWidget *parent)
    : QWidget(parent), mIdentifier(identifier), mShowAllConnections(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    mModel = new QStandardItemModel(0,3,this);
    mModel->setHorizontalHeaderLabels({ QStringLiteral("Sender"), QStringLiteral("Direction"), QStringLiteral("Message") });

    auto mDataView = new QTableView(this);
    mDataView->setModel(mModel);
    mDataView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    mDataView->horizontalHeader()->setStretchLastSection(true);

    layout->addWidget(mDataView);

    org::freedesktop::Akonadi::TracerNotification *iface = new org::freedesktop::Akonadi::TracerNotification(QString(), QStringLiteral("/tracing/notifications"), QDBusConnection::sessionBus(), this);

    connect(iface, &OrgFreedesktopAkonadiTracerNotificationInterface::connectionDataInput,
            this, &ConnectionPage::connectionDataInput);
    connect(iface, &OrgFreedesktopAkonadiTracerNotificationInterface::connectionDataOutput,
            this, &ConnectionPage::connectionDataOutput);
    connect(mDataView->horizontalHeader(), &QHeaderView::sectionResized,
            mDataView, &QTableView::resizeRowsToContents);
}

void ConnectionPage::connectionDataInput(const QString &identifier, const QString &msg)
{
    QString str = QStringLiteral("<font color=\"green\">%2</font>").arg(identifier) + QLatin1Char(' ');
    if (mShowAllConnections || identifier == mIdentifier) {
        str += QStringLiteral("<font color=\"red\">%1</font>").arg(msg.toHtmlEscaped());

        auto out = new QStandardItem(QStringLiteral("<-"));
        out->setForeground(Qt::red);
        mModel->appendRow(QList<QStandardItem*>() << new QStandardItem(identifier) << out <<  new QStandardItem(msg.trimmed()));
    }
}

void ConnectionPage::connectionDataOutput(const QString &identifier, const QString &msg)
{
    QString str = QStringLiteral("<font color=\"green\">%2</font>").arg(identifier) + QLatin1Char(' ');
    if (mShowAllConnections || identifier == mIdentifier) {
        str += msg.toHtmlEscaped();

        auto out = new QStandardItem(QStringLiteral("->"));
        out->setForeground(Qt::green);
        mModel->appendRow({new QStandardItem(identifier), out, new QStandardItem(msg.trimmed())});
    }
}

void ConnectionPage::showAllConnections(bool show)
{
    mShowAllConnections = show;
}

QString ConnectionPage::toHtml() const
{
    QString ret;
    int anz = mModel->rowCount();
    for (int row=0; row < anz; row++) {
        const auto &identifier = mModel->data(mModel->index(row,0)).toString();
        const auto &direction = mModel->data(mModel->index(row,1)).toString();
        const auto &msg = mModel->data(mModel->index(row,2)).toString();

        ret += identifier + direction + msg + QStringLiteral("\n");

    }
    return ret;
}

void ConnectionPage::clear()
{
    mModel->removeRows(0, mModel->rowCount());
}

