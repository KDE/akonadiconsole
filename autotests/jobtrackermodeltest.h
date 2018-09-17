/*
  Copyright (C) 2017-2018 Montel Laurent <montel@kde.org>
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
#ifndef JOBTRACKERMODELTEST_H
#define JOBTRACKERMODELTEST_H

#include <QObject>

class JobTracker;

class JobTrackerModelTest : public QObject
{
    Q_OBJECT
public:
    explicit JobTrackerModelTest(QObject *parent = nullptr);
    ~JobTrackerModelTest();
private Q_SLOTS:
    void initTestCase();
    void shouldBeEmpty();
    void shouldDisplayOneJob();
    void shouldSignalDataChanges();
    void shouldHandleReset();
    void shouldHandleDuplicateJob();
};

#endif // JOBTRACKERTEST_H
