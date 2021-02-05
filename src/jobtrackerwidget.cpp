/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2009 Till Adam <adam@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "jobtrackerwidget.h"
#include <QCheckBox>

#include "jobtrackerfilterproxymodel.h"
#include "jobtrackermodel.h"
#include "jobtrackersearchwidget.h"

#include <AkonadiWidgets/controlgui.h>

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

class JobTrackerWidget::Private
{
public:
    JobTrackerModel *model = nullptr;
    QTreeView *tv = nullptr;
    JobTrackerFilterProxyModel *filterProxyModel = nullptr;
    JobTrackerSearchWidget *searchLineEditWidget = nullptr;
};

JobTrackerWidget::JobTrackerWidget(const char *name, QWidget *parent, const QString &checkboxText)
    : QWidget(parent)
    , d(new Private)
{
    d->model = new JobTrackerModel(name, this);

    auto layout = new QVBoxLayout(this);

    auto enableCB = new QCheckBox(this);
    enableCB->setText(checkboxText);
    connect(enableCB, &QAbstractButton::toggled, d->model, &JobTrackerModel::setEnabled);
    layout->addWidget(enableCB);

    d->searchLineEditWidget = new JobTrackerSearchWidget(this);
    layout->addWidget(d->searchLineEditWidget);
    connect(d->searchLineEditWidget, &JobTrackerSearchWidget::searchTextChanged, this, &JobTrackerWidget::textFilterChanged);
    connect(d->searchLineEditWidget, &JobTrackerSearchWidget::columnChanged, this, &JobTrackerWidget::searchColumnChanged);
    connect(d->searchLineEditWidget, &JobTrackerSearchWidget::selectOnlyErrorChanged, this, &JobTrackerWidget::selectOnlyErrorChanged);
    d->filterProxyModel = new JobTrackerFilterProxyModel(this);
    d->filterProxyModel->setSourceModel(d->model);

    d->tv = new QTreeView(this);
    d->tv->setModel(d->filterProxyModel);
    d->tv->expandAll();
    d->tv->setAlternatingRowColors(true);
    d->tv->setContextMenuPolicy(Qt::CustomContextMenu);
    // too slow with many jobs:
    // tv->header()->setResizeMode( QHeaderView::ResizeToContents );
    connect(d->model, &JobTrackerModel::modelReset, d->tv, &QTreeView::expandAll);
    connect(d->tv, &QTreeView::customContextMenuRequested, this, &JobTrackerWidget::contextMenu);
    layout->addWidget(d->tv);
    d->model->setEnabled(false); // since it can be slow, default to off

    auto layout2 = new QHBoxLayout;
    QPushButton *button = new QPushButton(QStringLiteral("Save to file..."), this);
    connect(button, &QAbstractButton::clicked, this, &JobTrackerWidget::slotSaveToFile);
    layout2->addWidget(button);
    layout2->addStretch(1);
    layout->addLayout(layout2);

    Akonadi::ControlGui::widgetNeedsAkonadi(this);
}

JobTrackerWidget::~JobTrackerWidget()
{
    delete d;
}

void JobTrackerWidget::selectOnlyErrorChanged(bool state)
{
    d->filterProxyModel->setShowOnlyFailed(state);
    d->tv->expandAll();
}

void JobTrackerWidget::searchColumnChanged(int index)
{
    d->filterProxyModel->setSearchColumn(index);
    d->tv->expandAll();
}

void JobTrackerWidget::textFilterChanged(const QString &str)
{
    d->filterProxyModel->setFilterFixedString(str);
    d->tv->expandAll();
}

void JobTrackerWidget::contextMenu(const QPoint & /*pos*/)
{
    QMenu menu;
    menu.addAction(QStringLiteral("Clear View"), d->model, &JobTrackerModel::resetTracker);
    menu.addSeparator();
    menu.addAction(QStringLiteral("Copy Info"), this, &JobTrackerWidget::copyJobInfo);
    menu.addSeparator();
    menu.addAction(QStringLiteral("Collapse All"), this, &JobTrackerWidget::collapseAll);
    menu.addAction(QStringLiteral("Expand All"), this, &JobTrackerWidget::expandAll);

    menu.exec(QCursor::pos());
}

void JobTrackerWidget::expandAll()
{
    d->tv->expandAll();
}

void JobTrackerWidget::collapseAll()
{
    d->tv->collapseAll();
}

void JobTrackerWidget::copyJobInfo()
{
    QModelIndex index = d->tv->currentIndex();
    if (index.isValid()) {
        const QModelIndex newIndex = d->model->index(index.row(), 6, index.parent());
        QClipboard *cb = QApplication::clipboard();
        cb->setText(newIndex.data().toString(), QClipboard::Clipboard);
    }
}

void JobTrackerWidget::slotSaveToFile()
{
    const QString fileName = QFileDialog::getSaveFileName(this, QString(), QString(), QString());
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write("Job ID\t\tCreated\t\tWait Time\tJob Duration\tJob Type\t\tState\tInfo\n");

    writeRows(QModelIndex(), file, 0);

    file.close();
}

void JobTrackerWidget::writeRows(const QModelIndex &parent, QFile &file, int indentLevel)
{
    for (int row = 0; row < d->model->rowCount(parent); ++row) {
        QByteArray data;
        for (int tabs = 0; tabs < indentLevel; ++tabs) {
            data += '\t';
        }
        const int columnCount = d->model->columnCount(parent);
        for (int column = 0; column < columnCount; ++column) {
            const QModelIndex index = d->model->index(row, column, parent);
            data += index.data().toByteArray();
            if (column < columnCount - 1) {
                data += '\t';
            }
        }
        data += '\n';
        file.write(data);

        const QModelIndex index = d->model->index(row, 0, parent);
        writeRows(index, file, indentLevel + 1);
    }
}
