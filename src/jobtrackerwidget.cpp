/*
    This file is part of Akonadi.

    Copyright (c) 2009 Till Adam <adam@kde.org>

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

#include "jobtrackerwidget.h"
#include <QCheckBox>

#include "jobtrackermodel.h"
#include "jobtrackerfilterproxymodel.h"
#include "jobtrackersearchwidget.h"

#include <AkonadiWidgets/controlgui.h>


#include <QTreeView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>

class JobTrackerWidget::Private
{
public:
    JobTrackerModel *model = nullptr;
    QTreeView *tv = nullptr;
    JobTrackerFilterProxyModel *filterProxyModel = nullptr;
    JobTrackerSearchWidget *searchLineEditWidget = nullptr;
};

JobTrackerWidget::JobTrackerWidget(const char *name, QWidget *parent, const QString &checkboxText)
    : QWidget(parent),
      d(new Private)
{
    d->model = new JobTrackerModel(name, this);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QCheckBox *enableCB = new QCheckBox(this);
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
    d->model->setEnabled(false);   // since it can be slow, default to off

    QHBoxLayout *layout2 = new QHBoxLayout;
    QPushButton *button = new QPushButton(QStringLiteral("Save to file..."), this);
    connect(button, &QAbstractButton::clicked,
            this, &JobTrackerWidget::slotSaveToFile);
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

void JobTrackerWidget::contextMenu(const QPoint &/*pos*/)
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
