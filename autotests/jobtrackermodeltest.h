/*
  SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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
