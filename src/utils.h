/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef AKONADICONSOLE_UTILS_H_
#define AKONADICONSOLE_UTILS_H_

#include <QStringList>
#include <QDateTime>
#include <QStandardItem>

#include <akonadi/private/protocol_p.h>

template<typename T>
typename std::enable_if<std::is_integral<T>::value, QString>::type
toString(T num)
{
    return QString::number(num);
}

inline
QString toString(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

inline
QString toString(const QDateTime &dt)
{
    return dt.toString(Qt::ISODate);
}

inline
QString toString(const QString &str)
{
    return str;
}

inline
QString toString(const QByteArray &ba)
{
    return QString::fromUtf8(ba);
}

inline
QString toString(Akonadi::Tristate tristate)
{
    switch (tristate) {
    case Akonadi::Tristate::True:
        return QStringLiteral("true");
    case Akonadi::Tristate::False:
        return QStringLiteral("false");
    case Akonadi::Tristate::Undefined:
        return QStringLiteral("undefined");
    }
    return {};
}

template<typename T, template<typename> class Container>
QString toString(const Container<T> &set)
{
    QStringList rv;
    for (const auto &v : set) {
        rv << toString(v);
    }
    return rv.join(QStringLiteral(", "));
}

inline
void appendRow(QStandardItemModel *model, const QString &name, const QString &value)
{
    auto item = new QStandardItem(value);
    item->setToolTip(value);
    model->appendRow({ new QStandardItem(name), item });
}

inline
void appendRow(QStandardItem *parent, const QString &name, const QString &value)
{
    auto item = new QStandardItem(value);
    item->setToolTip(value);
    parent->appendRow({ new QStandardItem(name), item });
}

#endif
