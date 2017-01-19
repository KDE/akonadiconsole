/*
  Copyright (c) 2017 Montel Laurent <montel@kde.org>
  Copyright (c) 2017 David Faure <faure@kde.org>

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

#include "jobtrackermodeltest.h"
#include "jobtracker.h"
#include "jobtrackermodel.h"
#include <akonadi/private/instance_p.h>
#include <QSignalSpy>
#include <QTest>

static QString rowSpyToText(const QSignalSpy &spy)
{
    if (!spy.isValid()) {
        return QStringLiteral("THE SIGNALSPY IS INVALID!");
    }
    QString str;
    for (int i = 0; i < spy.count(); ++i) {
        str += spy.at(i).at(1).toString() + QLatin1Char(',') + spy.at(i).at(2).toString();
        if (i + 1 < spy.count()) {
            str += QLatin1Char(';');
        }
    }
    return str;
}

JobTrackerModelTest::JobTrackerModelTest(QObject *parent)
    : QObject(parent)
{
}

JobTrackerModelTest::~JobTrackerModelTest()
{
}

void JobTrackerModelTest::initTestCase()
{
    // Don't interfer with a running akonadiconsole
    Akonadi::Instance::setIdentifier("jobtrackertest");
}

void JobTrackerModelTest::shouldBeEmpty()
{
    JobTrackerModel model("jobtracker");
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 7);
}

void JobTrackerModelTest::shouldDisplayOneJob()
{
    // GIVEN
    JobTrackerModel model("jobtracker");
    const QString jobName("job1");
    QSignalSpy rowATBISpy(&model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowInsertedSpy(&model, SIGNAL(rowsInserted(QModelIndex,int,int)));
    int sessionRowCountBefore = -1;
    int jobRowCountBefore = -1;
    connect(&model, &QAbstractItemModel::rowsAboutToBeInserted,
    this, [&](const QModelIndex & parent) {
        if (!parent.isValid()) {
            sessionRowCountBefore = model.rowCount(parent);
        } else {
            jobRowCountBefore = model.rowCount(parent);
        }
    });

    // WHEN
    model.jobTracker().jobCreated("session1", jobName, QString(), "type1", "debugStr1");

    // THEN
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.index(0, 0).data().toString(), QStringLiteral("session1"));
    QCOMPARE(rowSpyToText(rowATBISpy), QStringLiteral("0,0;0,0"));
    QCOMPARE(rowSpyToText(rowInsertedSpy), QStringLiteral("0,0;0,0"));
    // rowsAboutToBeInserted is supposed to be emitted before the insert
    QCOMPARE(sessionRowCountBefore, 0);
    QCOMPARE(jobRowCountBefore, 0);
}

void JobTrackerModelTest::shouldSignalDataChanges()
{
    // GIVEN
    JobTrackerModel model("jobtracker");
    const QString jobName("job1");
    model.jobTracker().jobCreated("session1", jobName, QString(), "type1", "debugStr1");
    QSignalSpy dataChangedSpy(&model, &JobTrackerModel::dataChanged);

    // WHEN
    model.jobTracker().jobStarted(jobName);
    model.jobTracker().signalUpdates();

    // THEN
    QCOMPARE(dataChangedSpy.count(), 1);

    // AND WHEN
    model.jobTracker().jobEnded(jobName, QString());
    model.jobTracker().signalUpdates();

    // THEN
    QCOMPARE(dataChangedSpy.count(), 2);
}

void JobTrackerModelTest::shouldHandleReset()
{
    // GIVEN
    JobTrackerModel model("jobtracker");
    const QString jobName("job1");
    model.jobTracker().jobCreated("session1", jobName, QString(), "type1", "debugStr1");
    QSignalSpy modelATBResetSpy(&model, &JobTrackerModel::modelAboutToBeReset);
    QSignalSpy modelResetSpy(&model, &JobTrackerModel::modelReset);
    QSignalSpy dataChangedSpy(&model, &JobTrackerModel::dataChanged);

    // WHEN
    model.resetTracker();
    QCOMPARE(modelATBResetSpy.count(), 1);
    QCOMPARE(modelResetSpy.count(), 1);

    // AND then an update comes for that job which has been removed
    model.jobTracker().jobStarted(jobName);
    model.jobTracker().jobEnded(jobName, QString());
    model.jobTracker().signalUpdates();

    // THEN it should be ignored
    QCOMPARE(dataChangedSpy.count(), 0);
}

QTEST_MAIN(JobTrackerModelTest)
