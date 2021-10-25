/*
 This file is part of Akonadi.

 SPDX-FileCopyrightText: 2009 Till Adam <adam@kde.org>

 SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "libakonadiconsole_export.h"
#include <QAbstractItemModel>

#include <memory>

class JobTracker;
class JobTrackerModelPrivate;

class LIBAKONADICONSOLE_EXPORT JobTrackerModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit JobTrackerModel(const char *name, QObject *parent = nullptr);
    ~JobTrackerModel() override;

    JobTracker &jobTracker(); // for the unittest

    enum Roles { FailedIdRole = Qt::UserRole + 1 };

    enum Column {
        ColumnJobId,
        ColumnCreated,
        ColumnWaitTime,
        ColumnJobDuration,
        ColumnJobType,
        ColumnState,
        ColumnInfo,

        NumColumns // always last
    };

    /* QAIM API */
    QModelIndex index(int, int, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool isEnabled() const;

public Q_SLOTS:
    void resetTracker();
    void setEnabled(bool);

private Q_SLOTS:
    void jobAboutToBeAdded(int pos, int parentId);
    void jobAdded();
    void jobsUpdated(const QList<QPair<int, int>> &);

private:
    std::unique_ptr<JobTrackerModelPrivate> const d;
};

