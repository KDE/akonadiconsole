/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADIBROWSERMODEL_H
#define AKONADIBROWSERMODEL_H

#include <AkonadiCore/entitytreemodel.h>
#include <AkonadiCore/changerecorder.h>

#include <QSortFilterProxyModel>

using namespace Akonadi;

class AkonadiBrowserModel : public EntityTreeModel
{
    Q_OBJECT
public:
    explicit AkonadiBrowserModel(ChangeRecorder *monitor, QObject *parent = nullptr);
    ~AkonadiBrowserModel();

    enum ItemDisplayMode {
        GenericMode,
        MailMode,
        ContactsMode,
        CalendarMode
    };

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

    ItemDisplayMode m_itemDisplayMode;
};

class AkonadiBrowserSortModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AkonadiBrowserSortModel(AkonadiBrowserModel *browserModel, QObject *parent = nullptr);
    ~AkonadiBrowserSortModel();

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    AkonadiBrowserModel *mBrowserModel;
};

#endif

