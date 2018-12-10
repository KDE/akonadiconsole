/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2018  David Faure <faure@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "notificationfiltermodel.h"
#include "notificationmodel.h"

#include <KCheckComboBox>
#include <QStandardItemModel>

using KPIM::KCheckComboBox;

Q_DECLARE_METATYPE(Akonadi::ChangeNotification)

NotificationFilterModel::NotificationFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    mInvalidateTimer.setInterval(50);
    mInvalidateTimer.setSingleShot(true);
    connect(&mInvalidateTimer, &QTimer::timeout,
            this, &NotificationFilterModel::invalidateFilter);
}

NotificationFilterModel::~NotificationFilterModel()
{
}

void NotificationFilterModel::setTypeFilter(KPIM::KCheckComboBox *typeFilter)
{
    if (mTypeFilter) {
        mTypeFilter->disconnect(this);
    }
    mTypeFilter = typeFilter;
    connect(mTypeFilter, &KCheckComboBox::checkedItemsChanged,
            this, [this](const QStringList &items) {
                mCheckedTypes.clear();
                mCheckedTypes.reserve(items.count());
                // it sucks a bit that KCheckComboBox::checkedItems can't return a QVariantList instead of a QStringList
                for (const QString &item : mTypeFilter->checkedItems(Qt::UserRole)) {
                    mCheckedTypes.insert(static_cast<Akonadi::ChangeNotification::Type>(item.toInt()));
                }
                if (!mInvalidateTimer.isActive()) {
                    mInvalidateTimer.start();
                }
            });
}

bool NotificationFilterModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    const auto source_idx = sourceModel()->index(source_row, 0);
    const auto notification = sourceModel()->data(source_idx, NotificationModel::NotificationRole).value<Akonadi::ChangeNotification>();
    return mCheckedTypes.contains(notification.type());
}

void NotificationFilterModel::setSourceModel(QAbstractItemModel* model)
{
    if (sourceModel())
        disconnect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &NotificationFilterModel::slotRowsInserted);
    connect(model, &QAbstractItemModel::rowsInserted, this, &NotificationFilterModel::slotRowsInserted);
    QSortFilterProxyModel::setSourceModel(model);
}

void NotificationFilterModel::slotRowsInserted(const QModelIndex& source_parent, int start, int end)
{
    // insert new types (if any) into the type filter combo
    Q_ASSERT(!source_parent.isValid());
    for (int row = start; row <= end; ++row) {
        const QModelIndex source_idx = sourceModel()->index(start, NotificationModel::TypeColumn);
        const QString text = source_idx.data().toString();
        if (!mTypes.contains(text)) {
            mTypes.insert(text);
            auto *comboModel = qobject_cast<QStandardItemModel *>(mTypeFilter->model());
            Q_ASSERT(comboModel);
            auto item = new QStandardItem(text);
            const auto notification = sourceModel()->data(source_idx, NotificationModel::NotificationRole).value<Akonadi::ChangeNotification>();
            item->setData(int(notification.type()), Qt::UserRole);
            item->setCheckState(Qt::Checked);
            comboModel->appendRow(item);
        }
    }
}
