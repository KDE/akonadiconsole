/*
  SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "jobtrackermodeltest.h"
#include "jobtracker.h"
#include "jobtrackermodel.h"
//#include "modeltest.h"
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
    // Don't interfere with a running akonadiconsole
    Akonadi::Instance::setIdentifier(QStringLiteral("jobtrackertest"));
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
    //ModelTest modelTest(&model);
    const QString jobName(QStringLiteral("job1"));
    QSignalSpy rowATBISpy(&model, &QAbstractItemModel::rowsAboutToBeInserted);
    QSignalSpy rowInsertedSpy(&model, &QAbstractItemModel::rowsInserted);
    connect(&model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, [&](const QModelIndex &parent) {
        // rowsAboutToBeInserted is supposed to be emitted before the insert
        if (!parent.isValid()) {
            QCOMPARE(model.rowCount(), 0);
        } else {
            QCOMPARE(model.rowCount(parent), 0);
        }
    });
    connect(&model, &QAbstractItemModel::rowsInserted,
            this, [&](const QModelIndex &parent) {
        if (!parent.isValid()) {
            QCOMPARE(model.rowCount(), 1);
            QVERIFY(model.index(0, 0).isValid());
        } else {
            QCOMPARE(model.rowCount(parent), 1);
        }
    });

    // WHEN
    model.jobTracker().jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));

    // THEN
    QCOMPARE(model.rowCount(), 1);
    const QModelIndex sessionIndex = model.index(0, 0);
    QCOMPARE(sessionIndex.data().toString(), QStringLiteral("session1"));
    QCOMPARE(model.rowCount(sessionIndex), 1);
    const QModelIndex sessionCol1Index = model.index(0, 1);
    QVERIFY(sessionCol1Index.isValid());
    QCOMPARE(model.rowCount(sessionCol1Index), 0);
    const QModelIndex jobIndex = model.index(0, 0, sessionIndex);
    QCOMPARE(jobIndex.data().toString(), QStringLiteral("job1"));
    QCOMPARE(model.rowCount(jobIndex), 0);
    QCOMPARE(rowSpyToText(rowATBISpy), QStringLiteral("0,0;0,0"));
    QCOMPARE(rowSpyToText(rowInsertedSpy), QStringLiteral("0,0;0,0"));
}

void JobTrackerModelTest::shouldSignalDataChanges()
{
    // GIVEN
    JobTrackerModel model("jobtracker");
    const QString jobName(QStringLiteral("job1"));
    model.jobTracker().jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));
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
    const QString jobName(QStringLiteral("job1"));
    model.jobTracker().jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));
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

void JobTrackerModelTest::shouldHandleDuplicateJob()
{
    // GIVEN
    JobTrackerModel model("jobtracker");
    const QString jobName(QStringLiteral("job1"));
    model.jobTracker().jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));
    model.jobTracker().jobStarted(jobName);
    model.jobTracker().jobEnded(jobName, QString());
    model.jobTracker().signalUpdates();

    // WHEN
    QSignalSpy rowATBISpy(&model, &QAbstractItemModel::rowsAboutToBeInserted);
    QSignalSpy rowInsertedSpy(&model, &QAbstractItemModel::rowsInserted);
    model.jobTracker().jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));

    // THEN
    QCOMPARE(model.rowCount(), 1); // 1 session
    const QModelIndex sessionIndex = model.index(0, 0);
    QCOMPARE(rowSpyToText(rowATBISpy), QStringLiteral("1,1"));
    QCOMPARE(rowSpyToText(rowInsertedSpy), QStringLiteral("1,1"));
    QCOMPARE(model.rowCount(sessionIndex), 2);

    // AND WHEN
    model.jobTracker().jobStarted(jobName);
    model.jobTracker().jobEnded(jobName, QStringLiteral("error"));
    model.jobTracker().signalUpdates();

    // THEN
    QCOMPARE(model.index(0, JobTrackerModel::ColumnState, sessionIndex).data().toString(), QStringLiteral("Ended"));
    QCOMPARE(model.index(1, JobTrackerModel::ColumnState, sessionIndex).data().toString(), QStringLiteral("Failed: error"));
}

QTEST_GUILESS_MAIN(JobTrackerModelTest)
