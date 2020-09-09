/*
  SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "jobtrackersearchwidgettest.h"
#include "jobtrackersearchwidget.h"
#include <QTest>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSignalSpy>
#include <QComboBox>
#include <QCheckBox>

JobTrackerSearchWidgetTest::JobTrackerSearchWidgetTest(QObject *parent)
    : QObject(parent)
{
}

JobTrackerSearchWidgetTest::~JobTrackerSearchWidgetTest()
{
}

void JobTrackerSearchWidgetTest::shouldHaveDefaultValue()
{
    JobTrackerSearchWidget w;
    QHBoxLayout *mainLayout = w.findChild<QHBoxLayout *>(QStringLiteral("mainlayout"));
    QVERIFY(mainLayout);
    int top = -1;
    int bottom = -1;
    int left = -1;
    int right = -1;
    mainLayout->getContentsMargins(&left, &top, &right, &bottom);
    QCOMPARE(top, 0);
    QCOMPARE(bottom, 0);
    QCOMPARE(left, 0);
    QCOMPARE(right, 0);

    QLineEdit *mSearchLineEdit = w.findChild<QLineEdit *>(QStringLiteral("searchline"));
    QVERIFY(mSearchLineEdit);
    QVERIFY(mSearchLineEdit->isClearButtonEnabled());
    QVERIFY(mSearchLineEdit->text().isEmpty());

    QComboBox *mSelectColumn = w.findChild<QComboBox *>(QStringLiteral("selectcolumn"));
    QVERIFY(mSelectColumn);
    QCOMPARE(mSelectColumn->count(), 8);
    QCOMPARE(mSelectColumn->currentIndex(), 0);

    QCheckBox *mSelectOnlyError = w.findChild<QCheckBox *>(QStringLiteral("selectonlyerror"));
    QVERIFY(mSelectOnlyError);
    QVERIFY(!mSelectOnlyError->text().isEmpty());
    QCOMPARE(mSelectOnlyError->checkState(), Qt::Unchecked);
}

void JobTrackerSearchWidgetTest::shouldEmitSignal()
{
    JobTrackerSearchWidget w;
    QSignalSpy searchWidgetSignal(&w, &JobTrackerSearchWidget::searchTextChanged);
    QLineEdit *mSearchLineEdit = w.findChild<QLineEdit *>(QStringLiteral("searchline"));
    const QString foo = QStringLiteral("foo");
    mSearchLineEdit->setText(foo);
    QCOMPARE(searchWidgetSignal.count(), 1);
    QCOMPARE(searchWidgetSignal.at(0).at(0).toString(), foo);
    mSearchLineEdit->clear();
    QCOMPARE(searchWidgetSignal.count(), 2);
    QCOMPARE(searchWidgetSignal.at(1).at(0).toString(), QString());
}

void JobTrackerSearchWidgetTest::shouldEmitColumnChanged()
{
    JobTrackerSearchWidget w;
    QSignalSpy columnChangedSignal(&w, &JobTrackerSearchWidget::columnChanged);
    QComboBox *mSelectColumn = w.findChild<QComboBox *>(QStringLiteral("selectcolumn"));
    mSelectColumn->setCurrentIndex(2);
    QCOMPARE(columnChangedSignal.count(), 1);
}

void JobTrackerSearchWidgetTest::shouldEmitSelectOnlyErrorChanged()
{
    JobTrackerSearchWidget w;
    QSignalSpy selectOnlyErrorChangedSignal(&w, &JobTrackerSearchWidget::selectOnlyErrorChanged);
    QCheckBox *mSelectOnlyError = w.findChild<QCheckBox *>(QStringLiteral("selectonlyerror"));
    mSelectOnlyError->setChecked(true);
    QCOMPARE(selectOnlyErrorChangedSignal.count(), 1);
}

QTEST_MAIN(JobTrackerSearchWidgetTest)
