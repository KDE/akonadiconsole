/*
  Copyright (c) 2017 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "jobtrackersearchwidgettest.h"
#include "jobtrackersearchwidget.h"
#include <QTest>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSignalSpy>
#include <QComboBox>

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
    QCOMPARE(mainLayout->margin(), 0);

    QLineEdit *mSearchLineEdit = w.findChild<QLineEdit *>(QStringLiteral("searchline"));
    QVERIFY(mSearchLineEdit);
    QVERIFY(mSearchLineEdit->isClearButtonEnabled());
    QVERIFY(mSearchLineEdit->text().isEmpty());

    QComboBox *mSelectColumn = w.findChild<QComboBox *>(QStringLiteral("selectcolumn"));
    QVERIFY(mSelectColumn);
    QCOMPARE(mSelectColumn->count(), 8);
    QCOMPARE(mSelectColumn->currentIndex(), 0);
}

void JobTrackerSearchWidgetTest::shouldEmitSignal()
{
    JobTrackerSearchWidget w;
    QSignalSpy searchWidgetSignal(&w, SIGNAL(searchTextChanged(QString)));
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
    QSignalSpy columnChangedSignal(&w, SIGNAL(columnChanged(int)));
    QComboBox *mSelectColumn = w.findChild<QComboBox *>(QStringLiteral("selectcolumn"));
    mSelectColumn->setCurrentIndex(2);
    QCOMPARE(columnChangedSignal.count(), 1);
}

QTEST_MAIN(JobTrackerSearchWidgetTest)
