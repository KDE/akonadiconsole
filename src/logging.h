/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
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

    Q_INVOKABLE void message(qint64 timestamp, const QString &app, qint64 pid, int type,
                             const QString &category, const QString &file,
                             const QString &function, int line, int version,
                             const QString &msg);

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
