/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef QUERYDEBUGGER_H
#define QUERYDEBUGGER_H

#include <QMap>
#include <QScopedPointer>
#include <QVariant>
#include <QWidget>

class QDBusArgument;

namespace Ui
{
class QueryDebugger;
}

class QueryDebuggerModel;
class QueryTreeModel;
class OrgFreedesktopAkonadiStorageDebuggerInterface;

struct DbConnection {
    qint64 id;
    QString name;
    qint64 start;
    QString trxName;
    qint64 transactionStart;
};

Q_DECLARE_METATYPE(DbConnection)
Q_DECLARE_METATYPE(QVector<DbConnection>)

QDBusArgument &operator<<(QDBusArgument &arg, const DbConnection &con);
const QDBusArgument &operator>>(const QDBusArgument &arg, DbConnection &con);

class QueryDebugger : public QWidget
{
    Q_OBJECT

public:
    explicit QueryDebugger(QWidget *parent = nullptr);
    virtual ~QueryDebugger();

private Q_SLOTS:
    void debuggerToggled(bool on);
    void addQuery(double sequence,
                  qint64 connectionId,
                  qint64 timestamp,
                  uint duration,
                  const QString &query,
                  const QMap<QString, QVariant> &values,
                  int resultsCount,
                  const QList<QList<QVariant>> &result,
                  const QString &error);
    void queryTreeDoubleClicked(const QModelIndex &index);
    void clear();
    void saveTreeToFile();

private:
    QString variantToString(const QVariant &val);

    QScopedPointer<Ui::QueryDebugger> mUi;
    OrgFreedesktopAkonadiStorageDebuggerInterface *mDebugger = nullptr;

    QueryDebuggerModel *mQueryList = nullptr;
    QueryTreeModel *mQueryTree = nullptr;
};

#endif // QUERYDEBUGGER_H
