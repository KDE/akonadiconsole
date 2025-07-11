/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akonadibrowsermodel.h"
using namespace Qt::Literals::StringLiterals;

#include <KLocalizedString>
#include <KMime/Message>

#include <KContacts/Addressee>
#include <KContacts/ContactGroup>

#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>

using IncidencePtr = QSharedPointer<KCalendarCore::Incidence>;

class AkonadiBrowserModel::State
{
public:
    virtual ~State() = default;

    QStringList m_collectionHeaders;
    QStringList m_itemHeaders;

    [[nodiscard]] virtual QVariant entityData(const Item &item, int column, int role) const = 0;
};

class GenericState : public AkonadiBrowserModel::State
{
public:
    GenericState()
    {
        m_collectionHeaders << u"Collection"_s;
        m_itemHeaders << u"Id"_s << u"Remote Id"_s << QStringLiteral("GID") << QStringLiteral("MimeType");
    }

    ~GenericState() override = default;

    enum Columns {
        IdColumn = 0,
        RemoteIdColumn = 1,
        GIDColumn = 2,
        MimeTypeColumn = 3
    };

    [[nodiscard]] QVariant entityData(const Item &item, int column, int role) const override
    {
        if (Qt::DisplayRole != role) {
            return {};
        }

        switch (column) {
        case IdColumn:
            return item.id();
        case RemoteIdColumn:
            return item.remoteId();
        case GIDColumn:
            return item.gid();
        case MimeTypeColumn:
            return item.mimeType();
        }
        return {};
    }
};

class MailState : public AkonadiBrowserModel::State
{
public:
    MailState()
    {
        m_collectionHeaders << i18n("Collection");
        m_itemHeaders << i18n("Subject") << i18n("Sender") << i18n("Date");
    }

    ~MailState() override = default;

    [[nodiscard]] QVariant entityData(const Item &item, int column, int role) const override
    {
        if (Qt::DisplayRole != role) {
            return {};
        }

        if (!item.hasPayload<KMime::Message::Ptr>()) {
            return {};
        }
        const auto mail = item.payload<KMime::Message::Ptr>();

        // NOTE: remember to update AkonadiBrowserSortModel::lessThan if you insert/move columns
        switch (column) {
        case 0:
            if (mail->subject()) {
                return mail->subject()->asUnicodeString();
            } else {
                return u"(No subject)"_s;
            }
        case 1:
            if (mail->from()) {
                return mail->from()->asUnicodeString();
            } else {
                return QString();
            }
        case 2:
            if (mail->date()) {
                return mail->date()->asUnicodeString();
            } else {
                return QString();
            }
        }

        return {};
    }
};

class ContactsState : public AkonadiBrowserModel::State
{
public:
    ContactsState()
    {
        m_collectionHeaders << u"Collection"_s;
        m_itemHeaders << u"Given Name"_s << u"Family Name"_s << QStringLiteral("Email");
    }

    ~ContactsState() override = default;

    [[nodiscard]] QVariant entityData(const Item &item, int column, int role) const override
    {
        if (Qt::DisplayRole != role) {
            return {};
        }

        if (!item.hasPayload<KContacts::Addressee>() && !item.hasPayload<KContacts::ContactGroup>()) {
            return {};
        }

        if (item.hasPayload<KContacts::Addressee>()) {
            const auto addr = item.payload<KContacts::Addressee>();

            switch (column) {
            case 0:
                return addr.givenName();
            case 1:
                return addr.familyName();
            case 2:
                return addr.preferredEmail();
            }
            return {};
        }
        if (item.hasPayload<KContacts::ContactGroup>()) {
            switch (column) {
            case 0:
                const auto group = item.payload<KContacts::ContactGroup>();
                return group.name();
            }
            return {};
        }
        return {};
    }
};

class CalendarState : public AkonadiBrowserModel::State
{
public:
    CalendarState()
    {
        m_collectionHeaders << u"Collection"_s;
        m_itemHeaders << u"UID"_s << u"Summary"_s << QStringLiteral("DateTime start") << QStringLiteral("DateTime End") << u"Type"_s;
    }

    ~CalendarState() override = default;

    [[nodiscard]] QVariant entityData(const Item &item, int column, int role) const override
    {
        if (Qt::DisplayRole != role) {
            return {};
        }

        if (!item.hasPayload<IncidencePtr>()) {
            return {};
        }
        const auto incidence = item.payload<IncidencePtr>();
        // NOTE: remember to update AkonadiBrowserSortModel::lessThan if you insert/move columns
        switch (column) {
        case 0:
            return incidence->uid();
        case 1:
            return incidence->summary();
        case 2:
            return incidence->dtStart().toString();
        case 3:
            return incidence->dateTime(KCalendarCore::Incidence::RoleEnd).toString();
        case 4:
            return incidence->typeStr();
        default:
            break;
        }
        return {};
    }
};

AkonadiBrowserModel::AkonadiBrowserModel(ChangeRecorder *monitor, QObject *parent)
    : EntityTreeModel(monitor, parent)
{
    m_genericState = new GenericState();
    m_mailState = new MailState();
    m_contactsState = new ContactsState();
    m_calendarState = new CalendarState();

    m_currentState = m_genericState;
}

AkonadiBrowserModel::~AkonadiBrowserModel()
{
    delete m_genericState;
    delete m_mailState;
    delete m_contactsState;
    delete m_calendarState;
}

QVariant AkonadiBrowserModel::entityData(const Item &item, int column, int role) const
{
    QVariant var = m_currentState->entityData(item, column, role);
    if (!var.isValid()) {
        if (column < 1) {
            return EntityTreeModel::entityData(item, column, role);
        }
        return QString();
    }

    return var;
}

QVariant AkonadiBrowserModel::entityData(const Akonadi::Collection &collection, int column, int role) const
{
    return Akonadi::EntityTreeModel::entityData(collection, column, role);
}

int AkonadiBrowserModel::entityColumnCount(HeaderGroup headerGroup) const
{
    if (ItemListHeaders == headerGroup) {
        return m_currentState->m_itemHeaders.size();
    }

    if (CollectionTreeHeaders == headerGroup) {
        return m_currentState->m_collectionHeaders.size();
    }
    // Practically, this should never happen.
    return EntityTreeModel::entityColumnCount(headerGroup);
}

QVariant AkonadiBrowserModel::entityHeaderData(int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup) const
{
    if (section < 0) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return EntityTreeModel::entityHeaderData(section, orientation, role, headerGroup);
    }

    if (headerGroup == EntityTreeModel::CollectionTreeHeaders) {
        if (role == Qt::DisplayRole) {
            if (section >= m_currentState->m_collectionHeaders.size()) {
                return {};
            }
            return m_currentState->m_collectionHeaders.at(section);
        }
    } else if (headerGroup == EntityTreeModel::ItemListHeaders) {
        if (role == Qt::DisplayRole) {
            if (section >= m_currentState->m_itemHeaders.size()) {
                return {};
            }
            return m_currentState->m_itemHeaders.at(section);
        }
    }
    return EntityTreeModel::entityHeaderData(section, orientation, role, headerGroup);
}

AkonadiBrowserModel::ItemDisplayMode AkonadiBrowserModel::itemDisplayMode() const
{
    return m_itemDisplayMode;
}

void AkonadiBrowserModel::setItemDisplayMode(AkonadiBrowserModel::ItemDisplayMode itemDisplayMode)
{
    const int oldColumnCount = columnCount();
    m_itemDisplayMode = itemDisplayMode;
    AkonadiBrowserModel::State *newState = nullptr;
    switch (itemDisplayMode) {
    case MailMode:
        newState = m_mailState;
        break;
    case ContactsMode:
        newState = m_contactsState;
        break;
    case CalendarMode:
        newState = m_calendarState;
        break;
    case GenericMode:
    default:
        newState = m_genericState;
        break;
    }
    const int newColumnCount = qMax(newState->m_collectionHeaders.count(), newState->m_itemHeaders.count());

    // qCDebug(AKONADICONSOLE_LOG) << "column count changed from" << oldColumnCount << "to" << newColumnCount;
    if (newColumnCount > oldColumnCount) {
        beginInsertColumns(QModelIndex(), oldColumnCount, newColumnCount - 1);
        m_currentState = newState;
        endInsertColumns();
    } else if (newColumnCount < oldColumnCount) {
        beginRemoveColumns(QModelIndex(), newColumnCount, oldColumnCount - 1);
        m_currentState = newState;
        endRemoveColumns();
    } else {
        m_currentState = newState;
    }
    headerDataChanged(Qt::Horizontal, 0, newColumnCount - 1);

    // The above is not enough to see the new headers, because EntityMimeTypeFilterModel gets column count and headers from our data,
    // and doesn't listen to dataChanged/headerDataChanged...
    columnsChanged();
}

AkonadiBrowserSortModel::AkonadiBrowserSortModel(AkonadiBrowserModel *model, QObject *parent)
    : QSortFilterProxyModel(parent)
    , mBrowserModel(model)
{
}

AkonadiBrowserSortModel::~AkonadiBrowserSortModel() = default;

bool AkonadiBrowserSortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (mBrowserModel->itemDisplayMode() == AkonadiBrowserModel::CalendarMode) {
        if (left.column() == 2 || left.column() == 3) {
            const Item leftItem = left.data(EntityTreeModel::ItemRole).value<Item>();
            const Item rightItem = right.data(EntityTreeModel::ItemRole).value<Item>();
            if (!leftItem.hasPayload<IncidencePtr>() || !rightItem.hasPayload<IncidencePtr>()) {
                return false;
            }
            const auto leftInc = leftItem.payload<IncidencePtr>();
            const auto rightInc = rightItem.payload<IncidencePtr>();

            if (left.column() == 1) {
                return leftInc->dtStart() < rightInc->dtStart();
            } else if (left.column() == 2) {
                return leftInc->dateTime(KCalendarCore::IncidenceBase::RoleEnd) < rightInc->dateTime(KCalendarCore::IncidenceBase::RoleEnd);
            }
        }
    } else if (mBrowserModel->itemDisplayMode() == AkonadiBrowserModel::MailMode) {
        if (left.column() == 2) {
            const Item leftItem = left.data(EntityTreeModel::ItemRole).value<Item>();
            const Item rightItem = right.data(EntityTreeModel::ItemRole).value<Item>();
            if (!leftItem.hasPayload<KMime::Message::Ptr>() || !rightItem.hasPayload<KMime::Message::Ptr>()) {
                return false;
            }
            const auto leftMail = leftItem.payload<KMime::Message::Ptr>();
            const auto rightMail = rightItem.payload<KMime::Message::Ptr>();
            const KMime::Headers::Date *ldate = leftMail->date(false);
            const KMime::Headers::Date *rdate = rightMail->date(false);
            if (ldate && rdate) {
                return ldate->dateTime() < rdate->dateTime();
            } else {
                return false;
            }
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

#include "moc_akonadibrowsermodel.cpp"
