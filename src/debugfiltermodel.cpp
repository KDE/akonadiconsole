/*
    Copyright (c) 2018 Sandro Knauß <sknauss@kde.org>

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

#include "debugfiltermodel.h"
#include "debugmodel.h"
#include "akonadiconsole_debug.h"

#include <KCheckComboBox>

Q_DECLARE_METATYPE(DebugModel::Message)

using namespace KPIM;

DebugFilterModel::DebugFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    mInvalidateTimer.setInterval(50);
    mInvalidateTimer.setSingleShot(true);
    connect(&mInvalidateTimer, &QTimer::timeout,
            this, &DebugFilterModel::invalidate);
}

DebugFilterModel::~DebugFilterModel()
{
}

void DebugFilterModel::setSenderFilter(KCheckComboBox *senderFilter)
{
    if (mSenderFilter) {
        mSenderFilter->disconnect(this);
    }
    mSenderFilter = senderFilter;
    connect(mSenderFilter, &KCheckComboBox::checkedItemsChanged,
            this, [this](const QStringList &_items) {
                Q_UNUSED(_items);
                const auto items = mSenderFilter->checkedItems(DebugModel::IdentifierRole);
                mCheckedSenders.clear();
                mCheckedSenders.reserve(items.count());
                for (const auto &item : items) {
                    mCheckedSenders.insert(item);
                }
                if (!mInvalidateTimer.isActive()) {
                    mInvalidateTimer.start();
                }
            });
}

bool DebugFilterModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    const auto source_idx = sourceModel()->index(source_row, 0);
    const auto msg = sourceModel()->data(source_idx, DebugModel::MessageRole).value<DebugModel::Message>();
    return mCheckedSenders.contains(msg.sender);
}
