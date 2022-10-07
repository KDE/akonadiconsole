/*
    SPDX-FileCopyrightText: 2008 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityTreeModel>

#include <QSortFilterProxyModel>

using namespace Akonadi;

class AkonadiBrowserModel : public EntityTreeModel
{
    Q_OBJECT
public:
    explicit AkonadiBrowserModel(ChangeRecorder *monitor, QObject *parent = nullptr);
    ~AkonadiBrowserModel() override;

    enum ItemDisplayMode { GenericMode, MailMode, ContactsMode, CalendarMode };

    void setItemDisplayMode(ItemDisplayMode itemDisplayMode);
    ItemDisplayMode itemDisplayMode() const;

    QVariant entityHeaderData(int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup) const override;

    QVariant entityData(const Item &item, int column, int role) const override;
    QVariant entityData(const Collection &collection, int column, int role) const override;

    int entityColumnCount(HeaderGroup headerGroup) const override;

    class State;

Q_SIGNALS:
    void columnsChanged();

private:
    State *m_currentState = nullptr;
    State *m_genericState = nullptr;
    State *m_mailState = nullptr;
    State *m_contactsState = nullptr;
    State *m_calendarState = nullptr;

    ItemDisplayMode m_itemDisplayMode = GenericMode;
};

class AkonadiBrowserSortModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AkonadiBrowserSortModel(AkonadiBrowserModel *browserModel, QObject *parent = nullptr);
    ~AkonadiBrowserSortModel() override;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    AkonadiBrowserModel *const mBrowserModel;
};
