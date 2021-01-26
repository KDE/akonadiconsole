/*
 * SPDX-FileCopyrightText: 2018 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NOTIFICATIONFILTERMODEL_H
#define NOTIFICATIONFILTERMODEL_H

#include <QSortFilterProxyModel>

#include <AkonadiCore/ChangeNotification>

#include <QSet>
#include <QTimer>

namespace KPIM
{
class KCheckComboBox;
}

/**
 * Filtering for NotificationModel
 */
class NotificationFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit NotificationFilterModel(QObject *parent = nullptr);

    ~NotificationFilterModel();

    void setTypeFilter(KPIM::KCheckComboBox *typeFilter);

    void setSourceModel(QAbstractItemModel *model) override;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    void slotRowsInserted(const QModelIndex &source_parent, int start, int end);

    KPIM::KCheckComboBox *mTypeFilter = nullptr;
    QSet<Akonadi::ChangeNotification::Type> mCheckedTypes;

    QSet<QString> mTypes;
    QTimer mInvalidateTimer;
};

#endif // NOTIFICATIONFILTERMODEL_H
