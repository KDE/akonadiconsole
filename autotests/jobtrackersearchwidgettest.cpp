/*
  SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "jobtrackersearchwidgettest.h"
using namespace Qt::Literals::StringLiterals;

#include "jobtrackersearchwidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>

JobTrackerSearchWidgetTest::JobTrackerSearchWidgetTest(QObject *parent)
    : QObject(parent)
{
}

JobTrackerSearchWidgetTest::~JobTrackerSearchWidgetTest() = default;

void JobTrackerSearchWidgetTest::shouldHaveDefaultValue()
{
    JobTrackerSearchWidget w;
    auto mainLayout = w.findChild<QHBoxLayout *>(u"mainlayout"_s);
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

    auto mSearchLineEdit = w.findChild<QLineEdit *>(u"searchline"_s);
    QVERIFY(mSearchLineEdit);
    QVERIFY(mSearchLineEdit->isClearButtonEnabled());
    QVERIFY(mSearchLineEdit->text().isEmpty());

    auto mSelectColumn = w.findChild<QComboBox *>(u"selectcolumn"_s);
    QVERIFY(mSelectColumn);
    QCOMPARE(mSelectColumn->count(), 8);
    QCOMPARE(mSelectColumn->currentIndex(), 0);

    auto mSelectOnlyError = w.findChild<QCheckBox *>(u"selectonlyerror"_s);
    QVERIFY(mSelectOnlyError);
    QVERIFY(!mSelectOnlyError->text().isEmpty());
    QCOMPARE(mSelectOnlyError->checkState(), Qt::Unchecked);
}

void JobTrackerSearchWidgetTest::shouldEmitSignal()
{
    JobTrackerSearchWidget w;
    QSignalSpy searchWidgetSignal(&w, &JobTrackerSearchWidget::searchTextChanged);
    auto mSearchLineEdit = w.findChild<QLineEdit *>(u"searchline"_s);
    const QString foo = u"foo"_s;
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
    auto mSelectColumn = w.findChild<QComboBox *>(u"selectcolumn"_s);
    mSelectColumn->setCurrentIndex(2);
    QCOMPARE(columnChangedSignal.count(), 1);
}

void JobTrackerSearchWidgetTest::shouldEmitSelectOnlyErrorChanged()
{
    JobTrackerSearchWidget w;
    QSignalSpy selectOnlyErrorChangedSignal(&w, &JobTrackerSearchWidget::selectOnlyErrorChanged);
    auto mSelectOnlyError = w.findChild<QCheckBox *>(u"selectonlyerror"_s);
    mSelectOnlyError->setChecked(true);
    QCOMPARE(selectOnlyErrorChangedSignal.count(), 1);
}

QTEST_MAIN(JobTrackerSearchWidgetTest)

#include "moc_jobtrackersearchwidgettest.cpp"
