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

#include "jobtrackertest.h"
#include "jobtracker.h"
#include <akonadi/private/instance_p.h>
#include <QSignalSpy>
#include <QTest>

static QString intPairListToString(const QVariant &var)
{
    auto arg = var.value<QList<QPair<int, int>>>();
    QString ret;
    for (const auto &pair : arg) {
        if (!ret.isEmpty())
            ret += ' ';
        ret += QString::number(pair.first) + "," + QString::number(pair.second);
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
    // Don't interfer with a running akonadiconsole
    Akonadi::Instance::setIdentifier("jobtrackertest");

    qRegisterMetaType<QList<QPair<int,int>>>();
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
    const QString jobName("job1");
    QSignalSpy spyAboutToAdd(&tracker, &JobTracker::aboutToAdd);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobCreated("session1", jobName, QString(), "type1", "debugStr1");

    // THEN
    QCOMPARE(tracker.sessions().count(), 1);
    QCOMPARE(tracker.sessions().at(0), QStringLiteral("session1"));
    QCOMPARE(tracker.idForSession("session1"), -2);
    QCOMPARE(tracker.sessionForId(-2), QStringLiteral("session1"));
    QCOMPARE(tracker.info(jobName).id, jobName);
    QCOMPARE(tracker.info(jobName).state, JobInfo::Initial);
    QCOMPARE(tracker.idForJob(jobName), 42);
    QCOMPARE(tracker.jobForId(42), jobName);

    QCOMPARE(tracker.jobNames(-2), QStringList{jobName}); // job is child of session
    QCOMPARE(tracker.parentId(42), -2);
    QCOMPARE(tracker.jobNames(42), QStringList()); // no child
    QCOMPARE(tracker.parentId(-2), -1);

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
    const QString jobName("job1");
    tracker.jobCreated("session1", jobName, QString(), "type1", "debugStr1");
    tracker.signalUpdates();
    QSignalSpy spyAdded(&tracker, &JobTracker::added);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobStarted(jobName);

    // THEN
    QCOMPARE(tracker.info(jobName).state, JobInfo::Running);

    tracker.signalUpdates();

    QCOMPARE(spyAdded.count(), 0);
    QCOMPARE(spyUpdated.count(), 1);
    QCOMPARE(intPairListToString(spyUpdated.at(0).at(0)), QStringLiteral("0,-2"));
}

void JobTrackerTest::shouldHandleJobEnd()
{
    // GIVEN
    JobTracker tracker("jobtracker");
    const QString jobName("job1");
    tracker.jobCreated("session1", jobName, QString(), "type1", "debugStr1");
    tracker.jobStarted(jobName);
    tracker.signalUpdates();
    QSignalSpy spyAdded(&tracker, &JobTracker::added);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobEnded("job1", "errorString");

    // THEN
    QCOMPARE(tracker.info(jobName).state, JobInfo::Failed);
    QCOMPARE(tracker.info(jobName).error, QStringLiteral("errorString"));

    tracker.signalUpdates();

    QCOMPARE(spyAdded.count(), 0);
    QCOMPARE(spyUpdated.count(), 1);
    QCOMPARE(intPairListToString(spyUpdated.at(0).at(0)), QStringLiteral("0,-2"));
}

QTEST_MAIN(JobTrackerTest)
