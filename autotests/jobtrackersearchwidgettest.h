/*
  SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef JOBTRACKERSEARCHWIDGETTEST_H
#define JOBTRACKERSEARCHWIDGETTEST_H

#include <QObject>

class JobTrackerSearchWidgetTest : public QObject
{
    Q_OBJECT
public:
    explicit JobTrackerSearchWidgetTest(QObject *parent = nullptr);
    ~JobTrackerSearchWidgetTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldEmitSignal();
    void shouldEmitColumnChanged();
    void shouldEmitSelectOnlyErrorChanged();
};

#endif // JOBTRACKERSEARCHWIDGETTEST_H
