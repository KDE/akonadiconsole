/*
    SPDX-FileCopyrightText: 2018 Sandro Knauß <sknauss@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/


#ifndef DEBUGFILTERMODEL_H_
#define DEBUGFILTERMODEL_H_

#include <QSortFilterProxyModel>
#include <QSet>
#include <QTimer>

namespace KPIM {
class KCheckComboBox;
}

class DebugFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DebugFilterModel(QObject *parent = nullptr);
    ~DebugFilterModel();

    void setSenderFilter(KPIM::KCheckComboBox *appFilter);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    KPIM::KCheckComboBox *mSenderFilter = nullptr;
    QSet<QString> mCheckedSenders;
    QTimer mInvalidateTimer;
};

#endif
