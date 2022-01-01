/*
  SPDX-FileCopyrightText: 2017-2022 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>

class JobTracker;

class JobTrackerTest : public QObject
{
    Q_OBJECT
public:
    explicit JobTrackerTest(QObject *parent = nullptr);
    ~JobTrackerTest() override;
private Q_SLOTS:
    void initTestCase();
    void shouldBeEmpty();
    void shouldDisplayOneJob();
    void shouldHandleJobStart();
    void shouldHandleJobEnd();
};

