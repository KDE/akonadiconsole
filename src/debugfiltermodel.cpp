/*
    SPDX-FileCopyrightText: 2018 Sandro Knauß <sknauss@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "debugfiltermodel.h"
#include "akonadiconsole_debug.h"
#include "debugmodel.h"

#include <KCheckComboBox>
#include <chrono>

using namespace std::chrono_literals;

#ifndef COMPILE_WITH_UNITY_CMAKE_SUPPORT
Q_DECLARE_METATYPE(DebugModel::Message)
#endif

using namespace KPIM;

DebugFilterModel::DebugFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    mInvalidateTimer.setInterval(50ms);
    mInvalidateTimer.setSingleShot(true);
    connect(&mInvalidateTimer, &QTimer::timeout, this, &DebugFilterModel::invalidate);
}

DebugFilterModel::~DebugFilterModel() = default;

void DebugFilterModel::setSenderFilter(KCheckComboBox *senderFilter)
{
    if (mSenderFilter) {
        mSenderFilter->disconnect(this);
    }
    mSenderFilter = senderFilter;
    connect(mSenderFilter, &KCheckComboBox::checkedItemsChanged, this, [this](const QStringList &_items) {
        Q_UNUSED(_items)
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
