/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
    return rv.join(QLatin1String(", "));
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
