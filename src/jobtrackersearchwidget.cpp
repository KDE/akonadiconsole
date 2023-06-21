/*
  SPDX-FileCopyrightText: 2017-2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "jobtrackersearchwidget.h"

#include <KLocalizedString>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>

JobTrackerSearchWidget::JobTrackerSearchWidget(QWidget *parent)
    : QWidget(parent)
    , mSearchLineEdit(new QLineEdit(this))
    , mSelectColumn(new QComboBox(this))
    , mSelectOnlyError(new QCheckBox(i18n("Show Only Errors"), this))
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setObjectName(QStringLiteral("mainlayout"));
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mSearchLineEdit->setObjectName(QStringLiteral("searchline"));
    mSearchLineEdit->setClearButtonEnabled(true);
    mSearchLineEdit->setPlaceholderText(i18n("Search..."));
    mainLayout->addWidget(mSearchLineEdit);
    connect(mSearchLineEdit, &QLineEdit::textChanged, this, &JobTrackerSearchWidget::searchTextChanged);

    mSelectOnlyError->setObjectName(QStringLiteral("selectonlyerror"));
    mainLayout->addWidget(mSelectOnlyError);
    connect(mSelectOnlyError, &QCheckBox::toggled, this, &JobTrackerSearchWidget::selectOnlyErrorChanged);

    mSelectColumn->setObjectName(QStringLiteral("selectcolumn"));
    mainLayout->addWidget(mSelectColumn);
    mSelectColumn->addItem(i18n("All Columns"), -1);
    mSelectColumn->addItem(i18n("Job ID"), 0);
    mSelectColumn->addItem(i18n("Created"), 1);
    mSelectColumn->addItem(i18n("Wait time"), 2);
    mSelectColumn->addItem(i18n("Job duration"), 3);
    mSelectColumn->addItem(i18n("Job Type"), 4);
    mSelectColumn->addItem(i18n("State"), 5);
    mSelectColumn->addItem(i18n("Info"), 6);
    connect(mSelectColumn, &QComboBox::currentIndexChanged, this, &JobTrackerSearchWidget::slotColumnChanged);
}

JobTrackerSearchWidget::~JobTrackerSearchWidget() = default;

void JobTrackerSearchWidget::slotColumnChanged(int index)
{
    QVariant var = mSelectColumn->itemData(index);
    if (var.isValid()) {
        Q_EMIT columnChanged(var.toInt());
    }
}

#include "moc_jobtrackersearchwidget.cpp"
