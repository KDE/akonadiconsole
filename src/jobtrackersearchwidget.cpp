/*
  SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "jobtrackersearchwidget.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

JobTrackerSearchWidget::JobTrackerSearchWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainlayout"));
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mSearchLineEdit = new QLineEdit(this);
    mSearchLineEdit->setObjectName(QStringLiteral("searchline"));
    mSearchLineEdit->setClearButtonEnabled(true);
    mSearchLineEdit->setPlaceholderText(QStringLiteral("Search..."));
    mainLayout->addWidget(mSearchLineEdit);
    connect(mSearchLineEdit, &QLineEdit::textChanged, this, &JobTrackerSearchWidget::searchTextChanged);

    mSelectOnlyError = new QCheckBox(QStringLiteral("Show Only Errors"), this);
    mSelectOnlyError->setObjectName(QStringLiteral("selectonlyerror"));
    mainLayout->addWidget(mSelectOnlyError);
    connect(mSelectOnlyError, &QCheckBox::toggled, this, &JobTrackerSearchWidget::selectOnlyErrorChanged);


    mSelectColumn = new QComboBox(this);
    mSelectColumn->setObjectName(QStringLiteral("selectcolumn"));
    mainLayout->addWidget(mSelectColumn);
    mSelectColumn->addItem(QStringLiteral("All Columns"), -1);
    mSelectColumn->addItem(QStringLiteral("Job ID"), 0);
    mSelectColumn->addItem(QStringLiteral("Created"), 1);
    mSelectColumn->addItem(QStringLiteral("Wait time"), 2);
    mSelectColumn->addItem(QStringLiteral("Job duration"), 3);
    mSelectColumn->addItem(QStringLiteral("Job Type"), 4);
    mSelectColumn->addItem(QStringLiteral("State"), 5);
    mSelectColumn->addItem(QStringLiteral("Info"), 6);
    connect(mSelectColumn, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &JobTrackerSearchWidget::slotColumnChanged);
}

JobTrackerSearchWidget::~JobTrackerSearchWidget()
{

}

void JobTrackerSearchWidget::slotColumnChanged(int index)
{
    QVariant var = mSelectColumn->itemData(index);
    if (var.isValid()) {
        Q_EMIT columnChanged(var.toInt());
    }
}
