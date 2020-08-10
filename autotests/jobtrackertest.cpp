/*
  SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "jobtrackertest.h"
#include "jobtracker.h"
#include <akonadi/private/instance_p.h>
#include <QSignalSpy>
#include <QTest>

static QString intPairListToString(const QVariant &var)
{
    const auto arg = var.value<QList<QPair<int, int>>>();
    QString ret;
    for (const auto &pair : arg) {
        if (!ret.isEmpty()) {
            ret += QLatin1Char(' ');
        }
        ret += QString::number(pair.first) + QLatin1Char(',') + QString::number(pair.second);
    }
    return ret;
}

JobTrackerTest::JobTrackerTest(QObject *parent)
    : QObject(parent)
{
}

JobTrackerTest::~JobTrackerTest()
{
}

void JobTrackerTest::initTestCase()
{
    // Don't interfere with a running akonadiconsole
    Akonadi::Instance::setIdentifier(QStringLiteral("jobtrackertest"));

    qRegisterMetaType<QList<QPair<int, int>>>();
}

void JobTrackerTest::shouldBeEmpty()
{
    JobTracker tracker("jobtracker");
    QVERIFY(tracker.sessions().isEmpty());
    QCOMPARE(tracker.parentId(0), -1);
}

void JobTrackerTest::shouldDisplayOneJob()
{
    // GIVEN
    JobTracker tracker("jobtracker");
    const QString jobName(QStringLiteral("job1"));
    QSignalSpy spyAboutToAdd(&tracker, &JobTracker::aboutToAdd);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));

    // THEN
    QCOMPARE(tracker.sessions().count(), 1);
    QCOMPARE(tracker.sessions().at(0), QStringLiteral("session1"));
    QCOMPARE(tracker.idForSession(QStringLiteral("session1")), -2);
    QCOMPARE(tracker.sessionForId(-2), QStringLiteral("session1"));
    QCOMPARE(tracker.parentId(-2), -1);
    QCOMPARE(tracker.jobCount(-2), 1);
    QCOMPARE(tracker.jobIdAt(0, -2), 42); // job is child of session

    QCOMPARE(tracker.info(42).name, jobName);
    QCOMPARE(tracker.info(42).state, JobInfo::Initial);
    QCOMPARE(tracker.parentId(42), -2);
    QCOMPARE(tracker.rowForJob(42, -2), 0);
    QCOMPARE(tracker.jobCount(42), 0); // no child

    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAboutToAdd.at(0).at(0).toInt(), 0);
    QCOMPARE(spyAboutToAdd.at(0).at(1).toInt(), -1);
    QCOMPARE(spyAboutToAdd.at(1).at(0).toInt(), 0);
    QCOMPARE(spyAboutToAdd.at(1).at(1).toInt(), -2);
    QCOMPARE(spyUpdated.count(), 0);
}

void JobTrackerTest::shouldHandleJobStart()
{
    // GIVEN
    JobTracker tracker("jobtracker");
    const QString jobName(QStringLiteral("job1"));
    tracker.jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));
    tracker.signalUpdates();
    QSignalSpy spyAdded(&tracker, &JobTracker::added);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobStarted(jobName);

    // THEN
    QCOMPARE(tracker.info(42).state, JobInfo::Running);

    tracker.signalUpdates();

    QCOMPARE(spyAdded.count(), 0);
    QCOMPARE(spyUpdated.count(), 1);
    QCOMPARE(intPairListToString(spyUpdated.at(0).at(0)), QStringLiteral("0,-2"));
}

void JobTrackerTest::shouldHandleJobEnd()
{
    // GIVEN
    JobTracker tracker("jobtracker");
    const QString jobName(QStringLiteral("job1"));
    tracker.jobCreated(QStringLiteral("session1"), jobName, QString(), QStringLiteral("type1"), QStringLiteral("debugStr1"));
    tracker.jobStarted(jobName);
    tracker.signalUpdates();
    QSignalSpy spyAdded(&tracker, &JobTracker::added);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobEnded(QStringLiteral("job1"), QStringLiteral("errorString"));

    // THEN
    QCOMPARE(tracker.info(42).state, JobInfo::Failed);
    QCOMPARE(tracker.info(42).error, QStringLiteral("errorString"));

    tracker.signalUpdates();

    QCOMPARE(spyAdded.count(), 0);
    QCOMPARE(spyUpdated.count(), 1);
    QCOMPARE(intPairListToString(spyUpdated.at(0).at(0)), QStringLiteral("0,-2"));
}

QTEST_GUILESS_MAIN(JobTrackerTest)
