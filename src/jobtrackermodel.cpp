/*
 This file is part of Akonadi.

 SPDX-FileCopyrightText: 2009 KDAB
 SPDX-FileContributor: Till Adam <adam@kde.org>

 SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "jobtrackermodel.h"
#include "jobtracker.h"

#include <QColor>
#include <QFont>
#include <QModelIndex>
#include <QPair>
#include <QStringList>

#include <cassert>

class JobTrackerModel::Private
{
public:
    Private(const char *name, JobTrackerModel *_q)
        : q(_q)
        , tracker(name)
    {
    }

    int rowForParentId(int parentid) const
    {
        const int grandparentid = tracker.parentId(parentid);
        int row = -1;
        if (grandparentid == -1) {
            const QString session = tracker.sessionForId(parentid);
            if (!session.isEmpty()) {
                row = tracker.sessions().indexOf(session);
            }
        } else {
            // offset of the parent in the list of children of the grandparent
            row = tracker.rowForJob(parentid, grandparentid);
        }
        return row;
    }

private:
    JobTrackerModel *const q;

public:
    JobTracker tracker;
};

JobTrackerModel::JobTrackerModel(const char *name, QObject *parent)
    : QAbstractItemModel(parent)
    , d(new Private(name, this))
{
    connect(&d->tracker, &JobTracker::aboutToAdd, this, &JobTrackerModel::jobAboutToBeAdded);
    connect(&d->tracker, &JobTracker::added, this, &JobTrackerModel::jobAdded);
    connect(&d->tracker, &JobTracker::updated, this, &JobTrackerModel::jobsUpdated);
}

JobTrackerModel::~JobTrackerModel()
{
    delete d;
}

JobTracker &JobTrackerModel::jobTracker()
{
    return d->tracker;
}

QModelIndex JobTrackerModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column < 0 || column >= NumColumns) {
        return QModelIndex();
    }
    if (!parent.isValid()) { // session, at top level
        if (row < 0 || row >= d->tracker.sessions().size()) {
            return QModelIndex();
        }
        return createIndex(row, column, d->tracker.idForSession(d->tracker.sessions().at(row)));
    }
    if (parent.column() != 0) {
        return QModelIndex();
    }
    // job, i.e. non-toplevel
    const int jobCount = d->tracker.jobCount(parent.internalId());
    if (row < 0 || row >= jobCount) {
        return QModelIndex();
    }
    return createIndex(row, column, d->tracker.jobIdAt(row, parent.internalId()));
}

QModelIndex JobTrackerModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid()) {
        return QModelIndex();
    }

    const int parentid = d->tracker.parentId(idx.internalId());
    if (parentid == -1) {
        return QModelIndex(); // top level session
    }

    const int row = d->rowForParentId(parentid);
    if (row >= 0) {
        return createIndex(row, 0, parentid);
    } else {
        return QModelIndex();
    }
}

int JobTrackerModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return d->tracker.sessions().size();
    } else {
        if (parent.column() != 0) {
            return 0;
        }
        return d->tracker.jobCount(parent.internalId());
    }
}

int JobTrackerModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return NumColumns;
}

static QString formatTimeWithMsec(const QTime &time)
{
    return time.toString(QStringLiteral("HH:mm:ss.zzz t"));
}

static QString formatDurationWithMsec(qint64 msecs)
{
    QTime time(0, 0, 0);
    time = time.addMSecs(msecs);
    return time.toString(QStringLiteral("HH:mm:ss.zzz"));
}

QVariant JobTrackerModel::data(const QModelIndex &idx, int role) const
{
    // top level items are sessions
    if (!idx.parent().isValid()) {
        if (role == Qt::DisplayRole) {
            const QStringList sessions = d->tracker.sessions();
            if (idx.column() == 0 && idx.row() <= sessions.size()) {
                return sessions.at(idx.row());
            }
        }
    } else { // not top level, so a job or subjob
        const int id = idx.internalId();
        if (role != Qt::DisplayRole && role != Qt::ForegroundRole && role != Qt::FontRole && role != Qt::ToolTipRole && role != FailedIdRole) {
            // Avoid the QHash lookup for all other roles
            return QVariant();
        }
        const JobInfo info = d->tracker.info(id);
        if (role == Qt::DisplayRole) {
            switch (idx.column()) {
            case ColumnJobId:
                return info.name;
            case ColumnCreated:
                return formatTimeWithMsec(info.timestamp.time());
            case ColumnWaitTime:
                if (info.startedTimestamp.isNull() || info.timestamp.isNull()) {
                    return QString();
                }
                return formatDurationWithMsec(info.timestamp.msecsTo(info.startedTimestamp));
            case ColumnJobDuration:
                if (info.endedTimestamp.isNull() || info.startedTimestamp.isNull()) {
                    return QString();
                }
                return formatDurationWithMsec(info.startedTimestamp.msecsTo(info.endedTimestamp));
            case ColumnJobType:
                return info.type;
            case ColumnState:
                return info.stateAsString();
            case ColumnInfo:
                return info.debugString;
            }
        } else if (role == Qt::ForegroundRole) {
            if (info.state == JobInfo::Failed) {
                return QColor(Qt::red);
            }
        } else if (role == Qt::FontRole) {
            if (info.state == JobInfo::Running) {
                QFont f;
                f.setBold(true);
                return f;
            }
        } else if (role == Qt::ToolTipRole) {
            if (info.state == JobInfo::Failed) {
                return info.error;
            }
        } else if (role == FailedIdRole) {
            return info.state == JobInfo::Failed;
        }
    }
    return QVariant();
}

QVariant JobTrackerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case ColumnJobId:
                return QStringLiteral("Job ID");
            case ColumnCreated:
                return QStringLiteral("Created");
            case ColumnWaitTime:
                return QStringLiteral("Wait time"); // duration  (time started - time created)
            case ColumnJobDuration:
                return QStringLiteral("Job duration"); // duration (time ended - time started)
            case ColumnJobType:
                return QStringLiteral("Job Type");
            case ColumnState:
                return QStringLiteral("State");
            case ColumnInfo:
                return QStringLiteral("Info");
            }
        }
    }
    return QVariant();
}

void JobTrackerModel::resetTracker()
{
    beginResetModel();
    d->tracker.clear();
    endResetModel();
}

bool JobTrackerModel::isEnabled() const
{
    return d->tracker.isEnabled();
}

void JobTrackerModel::setEnabled(bool on)
{
    d->tracker.setEnabled(on);
}

void JobTrackerModel::jobAboutToBeAdded(int pos, int parentId)
{
    QModelIndex parentIdx;
    if (parentId != -1) {
        const int row = d->rowForParentId(parentId);
        if (row >= 0) {
            parentIdx = createIndex(row, 0, parentId);
        }
    }
    beginInsertRows(parentIdx, pos, pos);
}

void JobTrackerModel::jobAdded()
{
    endInsertRows();
}

void JobTrackerModel::jobsUpdated(const QList<QPair<int, int>> &jobs)
{
    // TODO group them by parent? It's likely that multiple jobs for the same
    // parent will come in in the same batch, isn't it?
    for (const auto &job : jobs) {
        const int pos = job.first;
        const int parentId = job.second;
        QModelIndex parentIdx;
        if (parentId != -1) {
            const int row = d->rowForParentId(parentId);
            if (row >= 0) {
                parentIdx = createIndex(row, 0, parentId);
            }
        }
        dataChanged(index(pos, 0, parentIdx), index(pos, 3, parentIdx));
    }
}
