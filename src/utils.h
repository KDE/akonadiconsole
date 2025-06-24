/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
using namespace Qt::Literals::StringLiterals;

#include <QStandardItem>
#include <QStringList>

#include <private/protocol_p.h>

template<typename T>
typename std::enable_if<std::is_integral<T>::value, QString>::type toString(T num)
{
    return QString::number(num);
}

inline QString toString(bool value)
{
    return value ? u"true"_s : u"false"_s;
}

inline QString toString(const QDateTime &dt)
{
    return dt.toString(Qt::ISODate);
}

inline QString toString(const QString &str)
{
    return str;
}

inline QString toString(const QByteArray &ba)
{
    return QString::fromUtf8(ba);
}

inline QString toString(Akonadi::Tristate tristate)
{
    switch (tristate) {
    case Akonadi::Tristate::True:
        return u"true"_s;
    case Akonadi::Tristate::False:
        return u"false"_s;
    case Akonadi::Tristate::Undefined:
        return u"undefined"_s;
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
    return rv.join(", "_L1);
}

inline void appendRow(QStandardItemModel *model, const QString &name, const QString &value)
{
    auto item = new QStandardItem(value);
    item->setToolTip(value);
    model->appendRow({new QStandardItem(name), item});
}

inline void appendRow(QStandardItem *parent, const QString &name, const QString &value)
{
    auto item = new QStandardItem(value);
    item->setToolTip(value);
    parent->appendRow({new QStandardItem(name), item});
}
