/*
 This file is part of Akonadi.

 SPDX-FileCopyrightText: 2009 KDAB
 SPDX-FileContributor: Till Adam <adam@kde.org>

 SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "jobtracker.h"
#include "akonadiconsole_debug.h"
#include "jobtrackeradaptor.h"
#include <KLocalizedString>
#include <QString>
#include <QStringList>
#include <akonadi/private/instance_p.h>

#include <cassert>
#include <chrono>

using namespace std::chrono_literals;
QString JobInfo::stateAsString() const
{
    switch (state) {
    case Initial:
        return i18n("Waiting");
    case Running:
        return i18n("Running");
    case Ended:
        return i18n("Ended");
    case Failed:
        return i18n("Failed: %1", error);
    default:
        return i18n("Unknown state!");
    }
}

class JobTrackerPrivate
{
public:
    explicit JobTrackerPrivate(JobTracker *_q)
        : lastId(42)
        , timer(_q)
        , disabled(false)
        , q(_q)
    {
        timer.setSingleShot(true);
        timer.setInterval(200ms);
        QObject::connect(&timer, &QTimer::timeout, q, &JobTracker::signalUpdates);
    }

    [[nodiscard]] bool isSession(int id) const
    {
        return id < -1;
    }

    void startUpdatedSignalTimer()
    {
        if (!timer.isActive() && !disabled) {
            timer.start();
        }
    }

    QStringList sessions;
    QHash<QString, int> nameToId;
    QHash<int, QList<int>> childJobs;
    QHash<int, JobInfo> infoList;
    int lastId;
    QTimer timer;
    bool disabled;
    QList<QPair<int, int>> unpublishedUpdates;

private:
    JobTracker *const q;
};

JobTracker::JobTracker(const char *name, QObject *parent)
    : QObject(parent)
    , d(new JobTrackerPrivate(this))
{
    new JobTrackerAdaptor(this);
    const QString suffix = Akonadi::Instance::identifier().isEmpty() ? QString() : QLatin1Char('-') + Akonadi::Instance::identifier();
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.akonadiconsole") + suffix);
    QDBusConnection::sessionBus().registerObject(QLatin1Char('/') + QLatin1StringView(name), this, QDBusConnection::ExportAdaptors);
}

JobTracker::~JobTracker() = default;

void JobTracker::jobCreated(const QString &session, const QString &jobName, const QString &parent, const QString &jobType, const QString &debugString)
{
    if (d->disabled || session.isEmpty() || jobName.isEmpty()) {
        return;
    }

    int parentId = parent.isEmpty() ? -1 /*for now*/ : idForJob(parent);

    if (!parent.isEmpty() && parentId == -1) {
        qCWarning(AKONADICONSOLE_LOG) << "JobTracker: Job" << jobName << "arrived before its parent" << parent << " jobType=" << jobType
                                      << "! Fix the library!";
        jobCreated(session, parent, QString(), QStringLiteral("dummy job type"), QString());
        parentId = idForJob(parent);
        assert(parentId != -1);
    }
    int sessionId = idForSession(session);
    // check if it's a new session, if so, add it
    if (sessionId == -1) {
        Q_EMIT aboutToAdd(d->sessions.count(), -1);
        d->sessions.append(session);
        Q_EMIT added();
        sessionId = idForSession(session);
    }
    if (parent.isEmpty()) {
        parentId = sessionId;
    }

    // deal with the job
    const int existingId = idForJob(jobName);
    if (existingId != -1) {
        if (d->infoList.value(existingId).state == JobInfo::Running) {
            qCDebug(AKONADICONSOLE_LOG) << "Job was already known and still running:" << jobName << "from"
                                        << d->infoList.value(existingId).timestamp.secsTo(QDateTime::currentDateTime()) << "s ago";
        }
        // otherwise it just means the pointer got reused... insert duplicate
    }

    assert(parentId != -1);
    QList<int> &kids = d->childJobs[parentId];
    const int pos = kids.size();

    Q_EMIT aboutToAdd(pos, parentId);

    const int id = d->lastId++;

    JobInfo info;
    info.name = jobName;
    info.parent = parentId;
    info.state = JobInfo::Initial;
    info.timestamp = QDateTime::currentDateTime();
    info.type = jobType;
    info.debugString = debugString;
    d->infoList.insert(id, info);
    d->nameToId.insert(jobName, id); // this replaces any previous entry for jobName, which is exactly what we want
    kids << id;

    Q_EMIT added();
}

void JobTracker::jobEnded(const QString &jobName, const QString &error)
{
    if (d->disabled) {
        return;
    }
    // this is called from dbus, so better be defensive
    const int jobId = idForJob(jobName);
    if (jobId == -1 || !d->infoList.contains(jobId)) {
        return;
    }

    JobInfo &info = d->infoList[jobId];
    if (error.isEmpty()) {
        info.state = JobInfo::Ended;
    } else {
        info.state = JobInfo::Failed;
        info.error = error;
    }
    info.endedTimestamp = QDateTime::currentDateTime();

    d->unpublishedUpdates << QPair<int, int>(d->childJobs[info.parent].size() - 1, info.parent);
    d->startUpdatedSignalTimer();
}

void JobTracker::jobStarted(const QString &jobName)
{
    if (d->disabled) {
        return;
    }
    // this is called from dbus, so better be defensive
    const int jobId = idForJob(jobName);
    if (jobId == -1 || !d->infoList.contains(jobId)) {
        return;
    }

    JobInfo &info = d->infoList[jobId];
    info.state = JobInfo::Running;
    info.startedTimestamp = QDateTime::currentDateTime();

    d->unpublishedUpdates << QPair<int, int>(d->childJobs[info.parent].size() - 1, info.parent);
    d->startUpdatedSignalTimer();
}

QStringList JobTracker::sessions() const
{
    return d->sessions;
}

QList<int> JobTracker::childJobs(int parentId) const
{
    return d->childJobs.value(parentId);
}

int JobTracker::jobCount(int parentId) const
{
    return d->childJobs.value(parentId).count();
}

int JobTracker::jobIdAt(int childPos, int parentId) const
{
    return d->childJobs.value(parentId).at(childPos);
}

// only works on jobs
int JobTracker::idForJob(const QString &job) const
{
    return d->nameToId.value(job, -1);
}

// To find a session, we take the offset in the list of sessions
// in order of appearance, add one, and make it negative. That
// way we can discern session ids from job ids and use -1 for invalid
int JobTracker::idForSession(const QString &session) const
{
    return (d->sessions.indexOf(session) + 2) * -1;
}

QString JobTracker::sessionForId(int _id) const
{
    const int id = (-_id) - 2;
    assert(d->sessions.size() > id);
    if (!d->sessions.isEmpty()) {
        return d->sessions.at(id);
    } else {
        return {};
    }
}

int JobTracker::parentId(int id) const
{
    if (d->isSession(id)) {
        return -1;
    } else {
        return d->infoList.value(id).parent;
    }
}

int JobTracker::rowForJob(int id, int parentId) const
{
    const QList<int> children = childJobs(parentId);
    // Simple version:
    // return children.indexOf(id);
    // But we can do faster since the vector is sorted
    return std::lower_bound(children.constBegin(), children.constEnd(), id) - children.constBegin();
}

JobInfo JobTracker::info(int id) const
{
    assert(d->infoList.contains(id));
    return d->infoList.value(id);
}

void JobTracker::clear()
{
    d->sessions.clear();
    d->nameToId.clear();
    d->childJobs.clear();
    d->infoList.clear();
    d->unpublishedUpdates.clear();
}

void JobTracker::setEnabled(bool on)
{
    d->disabled = !on;
}

bool JobTracker::isEnabled() const
{
    return !d->disabled;
}

void JobTracker::signalUpdates()
{
    if (!d->unpublishedUpdates.isEmpty()) {
        Q_EMIT updated(d->unpublishedUpdates);
        d->unpublishedUpdates.clear();
    }
}

#include "moc_jobtracker.cpp"
