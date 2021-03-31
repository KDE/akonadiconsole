/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2009 Till Adam <adam@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class QModelIndex;
class QFile;
class JobTrackerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit JobTrackerWidget(const char *name, QWidget *parent, const QString &checkboxText);
    ~JobTrackerWidget();
private Q_SLOTS:
    void contextMenu(const QPoint &pos);
    void slotSaveToFile();

private:
    void selectOnlyErrorChanged(bool state);
    void searchColumnChanged(int index);
    void expandAll();
    void copyJobInfo();
    void collapseAll();
    void textFilterChanged(const QString &str);
    void writeRows(const QModelIndex &parent, QFile &file, int indentLevel);

    class Private;
    Private *const d;
};

