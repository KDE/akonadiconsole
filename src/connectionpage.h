/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_CONNECTIONPAGE_H
#define AKONADICONSOLE_CONNECTIONPAGE_H

#include <QWidget>

class DebugModel;
class DebugFilterModel;
class QAbstractItemModel;
class QTableView;

namespace KPIM
{
class KCheckComboBox;
}

class ConnectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPage(const QString &identifier, QWidget *parent = nullptr);

    void showAllConnections(bool);

    QString toHtml() const;
    QString toHtmlFiltered() const;

public Q_SLOTS:
    void clear();
    void clearFiltered();

private Q_SLOTS:
    void connectionDataInput(const QString &, const QString &);
    void connectionDataOutput(const QString &, const QString &);

private:
    QString toHtml(QAbstractItemModel *model) const;

    DebugModel *mModel = nullptr;
    DebugFilterModel *mFilterModel = nullptr;
    QTableView *mDataView = nullptr;
    KPIM::KCheckComboBox *mSenderFilter = nullptr;
    QString mIdentifier;
    bool mShowAllConnections;
};

#endif
