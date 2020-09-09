/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_LOGGING_H_
#define AKONADICONSOLE_LOGGING_H_

#include <QWidget>

class QCheckBox;
class QTreeView;
class LoggingModel;
namespace KPIM {
class KCheckComboBox;
}

class Logging : public QWidget
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.kde.akonadiconsole.logger")

public:
    explicit Logging(QWidget *parent = nullptr);
    ~Logging();

    bool enabled() const;

    Q_INVOKABLE void message(qint64 timestamp, const QString &app, qint64 pid, int type, const QString &category, const QString &file, const QString &function, int line, int version, const QString &msg);

Q_SIGNALS:
    void enabledChanged(bool enabled);

private Q_SLOTS:
    void saveToFile();

private:
    QCheckBox *mEnabledCheckbox = nullptr;
    KPIM::KCheckComboBox *mAppFilter = nullptr;
    KPIM::KCheckComboBox *mTypeFilter = nullptr;
    KPIM::KCheckComboBox *mCategoryFilter = nullptr;
    QTreeView *mView = nullptr;
    LoggingModel *mModel = nullptr;
};

#endif
