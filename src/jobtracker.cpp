/*
 This file is part of Akonadi.

 Copyright (c) 2009 KDAB
 Author: Till Adam <adam@kde.org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 USA.
 */

#include "jobtracker.h"
#include "jobtrackeradaptor.h"
#include <akonadi/private/instance_p.h>
#include "akonadiconsole_debug.h"
#include <QString>
#include <QStringList>
#include <QPair>
#include <QList>

#include <cassert>

QString JobInfo::stateAsString() const
{
    switch (state) {
    case Initial:
        return QStringLiteral("Waiting");
    case Running:
        return QStringLiteral("Running");
    case Ended:
        return QStringLiteral("Ended");
    case Failed:
        return QStringLiteral("Failed: %1").arg(error);
    default:
        return QStringLiteral("Unknown state!");
    }
}

class JobTracker::Private
{
public:
    Private(JobTracker *_q)
        : lastId(42), timer(_q), disabled(false), q(_q)
    {
        timer.setSingleShot(true);
        timer.setInterval(200);
        connect(&timer, &QTimer::timeout, q, &JobTracker::signalUpdates);
    }

    bool isSession(int id) const
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
    QHash<QString, int> nameToSequence;
    QHash<int, QString> sequenceToName;
    QHash<QString, QStringList> jobs;
    QHash<QString, JobInfo> infoList;
    int lastId;
    QTimer timer;
    bool disabled;
    QList< QPair<int, int> > unpublishedUpdates;
private:
    JobTracker *const q;
};

JobTracker::JobTracker(const char *name, QObject *parent)
    : QObject(parent), d(new Private(this))
{
    new JobTrackerAdaptor(this);
    const QString suffix = Akonadi::Instance::identifier().isEmpty() ? QString() : QLatin1Char('-') + Akonadi::Instance::identifier();
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.akonadiconsole") + suffix);
    QDBusConnection::sessionBus().registerObject(QLatin1Char('/') + QLatin1String(name),
            this, QDBusConnection::ExportAdaptors);
}

JobTracker::~JobTracker()
{
    delete d;
}

void JobTracker::jobCreated(const QString &session, const QString &jobName, const QString &parent, const QString &jobType, const QString &debugString)
{
    if (d->disabled || session.isEmpty() || jobName.isEmpty()) {
        return;
    }

    if (!parent.isEmpty() && !d->jobs.contains(parent)) {
        qCWarning(AKONADICONSOLE_LOG) << "JobTracker: Job" << jobName << "arrived before its parent" << parent << " jobType=" << jobType << "! Fix the library!";
        jobCreated(session, parent, QString(), QStringLiteral("dummy job type"), QString());
    }
    // check if it's a new session, if so, add it
    if (!d->sessions.contains(session)) {
        emit aboutToAdd(d->sessions.count(), -1);
        d->sessions.append(session);
        d->jobs.insert(session, QStringList());
        emit added();
    }

    // deal with the job
    if (d->jobs.contains(jobName)) {
        if (d->infoList.value(jobName).state == JobInfo::Running) {
            qCDebug(AKONADICONSOLE_LOG) << "Job was already known and still running:" << jobName << "from" << d->infoList.value(jobName).timestamp.secsTo(QDateTime::currentDateTime()) << "s ago";
        }
        // otherwise it just means the pointer got reused... insert duplicate
    }

    const QString daddy = parent.isEmpty() ? session : parent;
    const int parentId = parent.isEmpty() ? idForSession(session) : idForJob(parent);
    assert(!daddy.isEmpty());
    QStringList &kids = d->jobs[daddy];
    const int pos = kids.size();

    emit aboutToAdd(pos, parentId);

    d->jobs.insert(jobName, QStringList());

    JobInfo info;
    info.name = jobName;
    info.parent = parentId;
    info.state = JobInfo::Initial;
    info.timestamp = QDateTime::currentDateTime();
    info.type = jobType;
    info.debugString = debugString;
    d->infoList.insert(jobName, info);
    const int id = d->lastId++;
    d->nameToSequence.insert(jobName, id);
    d->sequenceToName.insert(id, jobName);
    kids << jobName;

    emit added();
}

void JobTracker::jobEnded(const QString &job, const QString &error)
{
    // this is called from dbus, so better be defensive
    if (d->disabled || !d->jobs.contains(job) || !d->infoList.contains(job)) {
        return;
    }

    JobInfo &info = d->infoList[job];
    if (error.isEmpty()) {
        info.state = JobInfo::Ended;
    } else {
        info.state = JobInfo::Failed;
        info.error = error;
    }
    info.endedTimestamp = QDateTime::currentDateTime();

    d->unpublishedUpdates << QPair<int, int>(d->jobs[jobForId(info.parent)].size() - 1, info.parent);
    d->startUpdatedSignalTimer();
}

void JobTracker::jobStarted(const QString &job)
{
    // this is called from dbus, so better be defensive
    if (d->disabled || !d->jobs.contains(job) || !d->infoList.contains(job)) {
        return;
    }

    JobInfo &info = d->infoList[job];
    info.state = JobInfo::Running;
    info.startedTimestamp = QDateTime::currentDateTime();

    d->unpublishedUpdates << QPair<int, int>(d->jobs[jobForId(info.parent)].size() - 1, info.parent);
    d->startUpdatedSignalTimer();
}

QStringList JobTracker::sessions() const
{
    return d->sessions;
}

QStringList JobTracker::jobNames(int parentId) const
{
    if (d->isSession(parentId)) {
        return d->jobs.value(sessionForId(parentId));
    }
    return d->jobs.value(jobForId(parentId));
}

// only works on jobs
int JobTracker::jobCount(int parentId) const
{
    return d->jobs.value(jobForId(parentId)).count();
}

int JobTracker::jobIdAt(int childPos, int parentId) const
{
    // TODO: better data structure to make this faster
    const QStringList jobNames = d->jobs.value(jobForId(parentId));
    return idForJob(jobNames.at(childPos));
}

// only works on jobs
int JobTracker::idForJob(const QString &job) const
{
    assert(d->nameToSequence.contains(job));
    return d->nameToSequence.value(job);
}

QString JobTracker::jobForId(int id) const
{
    if (d->isSession(id)) {
        return sessionForId(id);
    }
    const QString jobName = d->sequenceToName.value(id);
    assert(!jobName.isEmpty());
    return jobName;
}

// To find a session, we take the offset in the list of sessions
// in order of appearance, add one, and make it negative. That
// way we can discern session ids from job ids and use -1 for invalid
int JobTracker::idForSession(const QString &session) const
{
    assert(d->sessions.contains(session));
    return (d->sessions.indexOf(session) + 2) * -1;
}

QString JobTracker::sessionForId(int _id) const
{
    const int id = (-_id) - 2;
    assert(d->sessions.size() > id);
    if (!d->sessions.isEmpty()) {
        return d->sessions.at(id);
    } else {
        return QString();
    }
}

int JobTracker::parentId(int id) const
{
    if (d->isSession(id)) {
        return -1;
    } else {
        const QString job = d->sequenceToName.value(id);
        return d->infoList.value(job).parent;
    }
}

int JobTracker::rowForJob(int id, int parentId) const
{
    return jobNames(parentId).indexOf(jobForId(id));
}

JobInfo JobTracker::info(int id) const
{
    return info(jobForId(id));
}

JobInfo JobTracker::info(const QString &job) const
{
    assert(d->infoList.contains(job));
    return d->infoList.value(job);
}

void JobTracker::clear()
{
    d->sessions.clear();
    d->nameToSequence.clear();
    d->sequenceToName.clear();
    d->jobs.clear();
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
