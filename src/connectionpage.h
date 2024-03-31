/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

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

    [[nodiscard]] QString toHtml() const;
    [[nodiscard]] QString toHtmlFiltered() const;

public Q_SLOTS:
    void clear();
    void clearFiltered();

private:
    void connectionDataInput(const QString &, const QString &);
    void connectionDataOutput(const QString &, const QString &);
    QString toHtml(QAbstractItemModel *model) const;

    DebugModel *mModel = nullptr;
    DebugFilterModel *mFilterModel = nullptr;
    QTableView *mDataView = nullptr;
    KPIM::KCheckComboBox *mSenderFilter = nullptr;
    const QString mIdentifier;
    bool mShowAllConnections = false;
};
