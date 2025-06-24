/*
  SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "jobtrackertest.h"
using namespace Qt::Literals::StringLiterals;

#include "jobtracker.h"
#include <QSignalSpy>
#include <QTest>
#include <private/instance_p.h>

static QString intPairListToString(const QVariant &var)
{
    const auto arg = var.value<QList<QPair<int, int>>>();
    QString ret;
    for (const auto &pair : arg) {
        if (!ret.isEmpty()) {
            ret += u' ';
        }
        ret += QString::number(pair.first) + u',' + QString::number(pair.second);
    }
    return ret;
}

JobTrackerTest::JobTrackerTest(QObject *parent)
    : QObject(parent)
{
}

JobTrackerTest::~JobTrackerTest() = default;

void JobTrackerTest::initTestCase()
{
    // Don't interfere with a running akonadiconsole
    Akonadi::Instance::setIdentifier(u"jobtrackertest"_s);

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
    const QString jobName(u"job1"_s);
    QSignalSpy spyAboutToAdd(&tracker, &JobTracker::aboutToAdd);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobCreated(u"session1"_s, jobName, QString(), u"type1"_s, QStringLiteral("debugStr1"));

    // THEN
    QCOMPARE(tracker.sessions().count(), 1);
    QCOMPARE(tracker.sessions().at(0), u"session1"_s);
    QCOMPARE(tracker.idForSession(u"session1"_s), -2);
    QCOMPARE(tracker.sessionForId(-2), u"session1"_s);
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
    const QString jobName(u"job1"_s);
    tracker.jobCreated(u"session1"_s, jobName, QString(), u"type1"_s, QStringLiteral("debugStr1"));
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
    QCOMPARE(intPairListToString(spyUpdated.at(0).at(0)), u"0,-2"_s);
}

void JobTrackerTest::shouldHandleJobEnd()
{
    // GIVEN
    JobTracker tracker("jobtracker");
    const QString jobName(u"job1"_s);
    tracker.jobCreated(u"session1"_s, jobName, QString(), u"type1"_s, QStringLiteral("debugStr1"));
    tracker.jobStarted(jobName);
    tracker.signalUpdates();
    QSignalSpy spyAdded(&tracker, &JobTracker::added);
    QSignalSpy spyUpdated(&tracker, &JobTracker::updated);

    // WHEN
    tracker.jobEnded(u"job1"_s, u"errorString"_s);

    // THEN
    QCOMPARE(tracker.info(42).state, JobInfo::Failed);
    QCOMPARE(tracker.info(42).error, u"errorString"_s);

    tracker.signalUpdates();

    QCOMPARE(spyAdded.count(), 0);
    QCOMPARE(spyUpdated.count(), 1);
    QCOMPARE(intPairListToString(spyUpdated.at(0).at(0)), u"0,-2"_s);
}

QTEST_GUILESS_MAIN(JobTrackerTest)

#include "moc_jobtrackertest.cpp"
