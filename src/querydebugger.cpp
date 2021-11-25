/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 * SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "querydebugger.h"
#include "storagedebuggerinterface.h"
#include "ui_querydebugger.h"
#include "ui_queryviewdialog.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QTableWidget>

#include <QDBusArgument>
#include <QDBusConnection>

#include <Akonadi/ControlGui>
#include <Akonadi/ServerManager>

#include <KColorScheme>

#include <algorithm>

Q_DECLARE_METATYPE(QList<QList<QVariant>>)

QDBusArgument &operator<<(QDBusArgument &arg, const DbConnection &con)
{
    arg.beginStructure();
    arg << con.id << con.name << con.start << con.trxName << con.transactionStart;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, DbConnection &con)
{
    arg.beginStructure();
    arg >> con.id >> con.name >> con.start >> con.trxName >> con.transactionStart;
    arg.endStructure();
    return arg;
}

struct QueryInfo {
    QString query;
    quint64 duration;
    quint64 calls;

    bool operator<(const QString &other) const
    {
        return query < other;
    }
};

Q_DECLARE_TYPEINFO(QueryInfo, Q_MOVABLE_TYPE);

class QueryTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum RowType { Connection, Transaction, Query };

private:
    class Node
    {
    public:
        virtual ~Node() = default;

        Node *parent;
        RowType type;
        qint64 start;
        uint duration;
    };

    class QueryNode : public Node
    {
    public:
        QString query;
        QString error;
        QMap<QString, QVariant> values;
        QList<QList<QVariant>> results;
        int resultsCount;
    };

    class TransactionNode : public QueryNode
    {
    public:
        ~TransactionNode() override
        {
            qDeleteAll(queries);
        }

        enum TransactionType { Begin, Commit, Rollback };
        TransactionType transactionType;
        QVector<QueryNode *> queries;
    };

    class ConnectionNode : public Node
    {
    public:
        ~ConnectionNode() override
        {
            qDeleteAll(queries);
        }

        QString name;
        QVector<Node *> queries; // FIXME: Why can' I use QVector<Query*> here??
    };

public:
    enum { RowTypeRole = Qt::UserRole + 1, QueryRole, QueryResultsCountRole, QueryResultsRole, QueryValuesRole };

    QueryTreeModel(QObject *parent)
        : QAbstractItemModel(parent)
    {
    }

    ~QueryTreeModel() override
    {
        qDeleteAll(mConnections);
    }

    void clear()
    {
        beginResetModel();
        qDeleteAll(mConnections);
        mConnections.clear();
        endResetModel();
    }

    void addConnection(qint64 id, const QString &name, qint64 timestamp)
    {
        auto con = new ConnectionNode;
        con->parent = nullptr;
        con->type = Connection;
        con->name = name;
        con->start = timestamp;
        beginInsertRows(QModelIndex(), mConnections.count(), mConnections.count());
        mConnections << con;
        mConnectionById.insert(id, con);
        endInsertRows();
    }

    void updateConnection(qint64 id, const QString &name)
    {
        auto con = mConnectionById.value(id);
        if (!con) {
            return;
        }

        con->name = name;
        const QModelIndex index = createIndex(mConnections.indexOf(con), columnCount() - 1, con);
        Q_EMIT dataChanged(index, index.sibling(index.row(), 5));
    }

    void addTransaction(qint64 connectionId, const QString &name, qint64 timestamp, uint duration, const QString &error)
    {
        auto con = mConnectionById.value(connectionId);
        if (!con) {
            return;
        }

        auto trx = new TransactionNode;
        trx->query = name;
        trx->parent = con;
        trx->type = Transaction;
        trx->start = timestamp;
        trx->duration = duration;
        trx->transactionType = TransactionNode::Begin;
        trx->error = error.trimmed();
        const QModelIndex conIdx = createIndex(mConnections.indexOf(con), 0, con);
        beginInsertRows(conIdx, con->queries.count(), con->queries.count());
        con->queries << trx;
        endInsertRows();
    }

    void closeTransaction(qint64 connectionId, bool commit, qint64 timestamp, uint, const QString &error)
    {
        auto con = mConnectionById.value(connectionId);
        if (!con) {
            return;
        }

        // Find the last open transaction and change it to closed
        for (int i = con->queries.count() - 1; i >= 0; i--) {
            Node *node = con->queries[i];
            if (node->type == Transaction) {
                auto trx = static_cast<TransactionNode *>(node);
                if (trx->transactionType != TransactionNode::Begin) {
                    continue;
                }

                trx->transactionType = commit ? TransactionNode::Commit : TransactionNode::Rollback;
                trx->duration = timestamp - trx->start;
                trx->error = error.trimmed();

                const QModelIndex trxIdx = createIndex(i, 0, trx);
                Q_EMIT dataChanged(trxIdx, trxIdx.sibling(trxIdx.row(), columnCount() - 1));
                return;
            }
        }
    }

    void addQuery(qint64 connectionId,
                  qint64 timestamp,
                  uint duration,
                  const QString &queryStr,
                  const QMap<QString, QVariant> &values,
                  int resultsCount,
                  const QList<QList<QVariant>> &results,
                  const QString &error)
    {
        auto con = mConnectionById.value(connectionId);
        if (!con) {
            return;
        }

        auto query = new QueryNode;
        query->type = Query;
        query->start = timestamp;
        query->duration = duration;
        query->query = queryStr;
        query->values = values;
        query->resultsCount = resultsCount;
        query->results = results;
        query->error = error.trimmed();

        if (!con->queries.isEmpty() && con->queries.last()->type == Transaction
            && static_cast<TransactionNode *>(con->queries.last())->transactionType == TransactionNode::Begin) {
            auto trx = static_cast<TransactionNode *>(con->queries.last());
            query->parent = trx;
            beginInsertRows(createIndex(con->queries.indexOf(trx), 0, trx), trx->queries.count(), trx->queries.count());
            trx->queries << query;
            endInsertRows();
        } else {
            query->parent = con;
            beginInsertRows(createIndex(mConnections.indexOf(con), 0, con), con->queries.count(), con->queries.count());
            con->queries << query;
            endInsertRows();
        }
    }

    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent) const override
    {
        if (!parent.isValid()) {
            return mConnections.count();
        }

        Node *node = reinterpret_cast<Node *>(parent.internalPointer());
        switch (node->type) {
        case Connection:
            return static_cast<ConnectionNode *>(node)->queries.count();
        case Transaction:
            return static_cast<TransactionNode *>(node)->queries.count();
        case Query:
            return 0;
        }

        Q_UNREACHABLE();
    }

    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return 5;
    }

    Q_REQUIRED_RESULT QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid() || !child.internalPointer()) {
            return {};
        }

        Node *childNode = reinterpret_cast<Node *>(child.internalPointer());
        // childNode is a Connection
        if (!childNode->parent) {
            return {};
        }

        // childNode is a query in transaction
        if (childNode->parent->parent) {
            auto connection = static_cast<ConnectionNode *>(childNode->parent->parent);
            const int trxIdx = connection->queries.indexOf(childNode->parent);
            return createIndex(trxIdx, 0, childNode->parent);
        } else {
            // childNode is a query without transaction or a transaction
            return createIndex(mConnections.indexOf(static_cast<ConnectionNode *>(childNode->parent)), 0, childNode->parent);
        }
    }

    Q_REQUIRED_RESULT QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        if (!parent.isValid()) {
            if (row < mConnections.count()) {
                return createIndex(row, column, mConnections.at(row));
            } else {
                return {};
            }
        }

        Node *parentNode = reinterpret_cast<Node *>(parent.internalPointer());
        switch (parentNode->type) {
        case Connection:
            if (row < static_cast<ConnectionNode *>(parentNode)->queries.count()) {
                return createIndex(row, column, static_cast<ConnectionNode *>(parentNode)->queries.at(row));
            } else {
                return {};
            }
        case Transaction:
            if (row < static_cast<TransactionNode *>(parentNode)->queries.count()) {
                return createIndex(row, column, static_cast<TransactionNode *>(parentNode)->queries.at(row));
            } else {
                return {};
            }
        case Query:
            // Query can never have children
            return {};
        }

        Q_UNREACHABLE();
    }

    Q_REQUIRED_RESULT QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }

        switch (section) {
        case 0:
            return QStringLiteral("Name / Query");
        case 1:
            return QStringLiteral("Started");
        case 2:
            return QStringLiteral("Ended");
        case 3:
            return QStringLiteral("Duration");
        case 4:
            return QStringLiteral("Error");
        }

        return {};
    }

    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) {
            return {};
        }

        Node *node = reinterpret_cast<Node *>(index.internalPointer());
        if (role == RowTypeRole) {
            return node->type;
        } else {
            switch (node->type) {
            case Connection:
                return connectionData(static_cast<ConnectionNode *>(node), index.column(), role);
            case Transaction:
                return transactionData(static_cast<TransactionNode *>(node), index.column(), role);
            case Query:
                return queryData(static_cast<QueryNode *>(node), index.column(), role);
            }
        }

        Q_UNREACHABLE();
    }

    void dumpRow(QFile &file, const QModelIndex &idx, int depth)
    {
        if (idx.isValid()) {
            QTextStream stream(&file);
            stream << QStringLiteral("  |").repeated(depth) << QLatin1String("- ");

            Node *node = reinterpret_cast<Node *>(idx.internalPointer());
            switch (node->type) {
            case Connection: {
                auto con = static_cast<ConnectionNode *>(node);
                stream << con->name << "    " << fromMSecsSinceEpoch(con->start);
                break;
            }
            case Transaction: {
                auto trx = static_cast<TransactionNode *>(node);
                stream << idx.data(Qt::DisplayRole).toString() << "    " << fromMSecsSinceEpoch(trx->start);
                if (trx->transactionType > TransactionNode::Begin) {
                    stream << " - " << fromMSecsSinceEpoch(trx->start + trx->duration);
                }
                break;
            }
            case Query: {
                auto query = static_cast<QueryNode *>(node);
                stream << query->query << "    " << fromMSecsSinceEpoch(query->start) << ", took " << query->duration << " ms";
                break;
            }
            }

            if (node->type >= Transaction) {
                auto query = static_cast<QueryNode *>(node);
                if (!query->error.isEmpty()) {
                    stream << '\n' << QStringLiteral("  |").repeated(depth) << QStringLiteral("  Error: ") << query->error;
                }
            }

            stream << '\n';
        }

        for (int i = 0, c = rowCount(idx); i < c; ++i) {
            dumpRow(file, index(i, 0, idx), depth + 1);
        }
    }

private:
    Q_REQUIRED_RESULT QString fromMSecsSinceEpoch(qint64 msecs) const
    {
        return QDateTime::fromMSecsSinceEpoch(msecs).toString(QStringLiteral("dd.MM.yyyy HH:mm:ss.zzz"));
    }

    QVariant connectionData(ConnectionNode *connection, int column, int role) const
    {
        if (role != Qt::DisplayRole) {
            return {};
        }

        switch (column) {
        case 0:
            return connection->name;
        case 1:
            return fromMSecsSinceEpoch(connection->start);
        }

        return {};
    }

    QVariant transactionData(TransactionNode *transaction, int column, int role) const
    {
        if (role == Qt::DisplayRole && column == 0) {
            QString mode;
            switch (transaction->transactionType) {
            case TransactionNode::Begin:
                mode = QStringLiteral("BEGIN");
                break;
            case TransactionNode::Commit:
                mode = QStringLiteral("COMMIT");
                break;
            case TransactionNode::Rollback:
                mode = QStringLiteral("ROLLBACK");
                break;
            }
            return QStringLiteral("%1 %2").arg(mode, transaction->query);
        } else {
            return queryData(transaction, column, role);
        }
    }

    QVariant queryData(QueryNode *query, int column, int role) const
    {
        switch (role) {
        case Qt::BackgroundRole:
            if (!query->error.isEmpty()) {
                return KColorScheme(QPalette::Normal).background(KColorScheme::NegativeBackground).color();
            }
            break;
        case Qt::DisplayRole:
            switch (column) {
            case 0:
                return query->query;
            case 1:
                return fromMSecsSinceEpoch(query->start);
            case 2:
                return fromMSecsSinceEpoch(query->start + query->duration);
            case 3:
                return QTime(0, 0, 0).addMSecs(query->duration).toString(QStringLiteral("HH:mm:ss.zzz"));
            case 4:
                return query->error;
            }
            break;
        case QueryRole:
            return query->query;
        case QueryResultsCountRole:
            return query->resultsCount;
        case QueryResultsRole:
            return QVariant::fromValue(query->results);
        case QueryValuesRole:
            return query->values;
        }

        return {};
    }

    QVector<ConnectionNode *> mConnections;
    QHash<qint64, ConnectionNode *> mConnectionById;
};

class QueryDebuggerModel : public QAbstractListModel
{
    Q_OBJECT
public:
    QueryDebuggerModel(QObject *parent)
        : QAbstractListModel(parent)
    {
        mSpecialRows[TOTAL].query = QStringLiteral("TOTAL");
        mSpecialRows[TOTAL].duration = 0;
        mSpecialRows[TOTAL].calls = 0;
    }

    enum SPECIAL_ROWS { TOTAL, NUM_SPECIAL_ROWS };
    enum COLUMNS { DurationColumn, CallsColumn, AvgDurationColumn, QueryColumn, NUM_COLUMNS };

    Q_REQUIRED_RESULT QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Vertical || section < 0 || section >= NUM_COLUMNS || (role != Qt::DisplayRole && role != Qt::ToolTipRole)) {
            return {};
        }

        if (section == QueryColumn) {
            return QStringLiteral("Query");
        } else if (section == DurationColumn) {
            return QStringLiteral("Duration [ms]");
        } else if (section == CallsColumn) {
            return QStringLiteral("Calls");
        } else if (section == AvgDurationColumn) {
            return QStringLiteral("Avg. Duration [ms]");
        }

        return {};
    }

    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole && role != Qt::ToolTipRole) {
            return {};
        }

        const int row = index.row();
        if (row < 0 || row >= rowCount(index.parent())) {
            return {};
        }
        const int column = index.column();
        if (column < 0 || column >= NUM_COLUMNS) {
            return {};
        }

        const QueryInfo &info = (row < NUM_SPECIAL_ROWS) ? mSpecialRows[row] : mQueries.at(row - NUM_SPECIAL_ROWS);

        if (role == Qt::ToolTipRole) {
            return QString(QLatin1String("<qt>") + info.query + QLatin1String("</qt>"));
        }

        if (column == QueryColumn) {
            return info.query;
        } else if (column == DurationColumn) {
            return info.duration;
        } else if (column == CallsColumn) {
            return info.calls;
        } else if (column == AvgDurationColumn) {
            return float(info.duration) / info.calls;
        }

        return {};
    }

    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (!parent.isValid()) {
            return mQueries.size() + NUM_SPECIAL_ROWS;
        } else {
            return 0;
        }
    }

    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (!parent.isValid()) {
            return NUM_COLUMNS;
        } else {
            return 0;
        }
    }

    void addQuery(const QString &query, uint duration)
    {
        QVector<QueryInfo>::iterator it = std::lower_bound(mQueries.begin(), mQueries.end(), query);

        const int row = std::distance(mQueries.begin(), it) + NUM_SPECIAL_ROWS;

        if (it != mQueries.end() && it->query == query) {
            ++(it->calls);
            it->duration += duration;

            Q_EMIT dataChanged(index(row, DurationColumn), index(row, AvgDurationColumn));
        } else {
            beginInsertRows(QModelIndex(), row, row);
            QueryInfo info;
            info.query = query;
            info.duration = duration;
            info.calls = 1;
            mQueries.insert(it, info);
            endInsertRows();
        }

        mSpecialRows[TOTAL].duration += duration;
        ++mSpecialRows[TOTAL].calls;
        Q_EMIT dataChanged(index(TOTAL, DurationColumn), index(TOTAL, AvgDurationColumn));
    }

    void clear()
    {
        beginResetModel();
        mQueries.clear();
        mSpecialRows[TOTAL].duration = 0;
        mSpecialRows[TOTAL].calls = 0;
        endResetModel();
    }

private:
    QVector<QueryInfo> mQueries;
    QueryInfo mSpecialRows[NUM_SPECIAL_ROWS];
};

class QueryViewDialog : public QDialog
{
    Q_OBJECT
public:
    QueryViewDialog(const QString &query,
                    const QMap<QString, QVariant> &values,
                    int resultsCount,
                    const QList<QList<QVariant>> &results,
                    QWidget *parent = nullptr)
        : QDialog(parent)
        , mUi(new Ui::QueryViewDialog)
    {
        mUi->setupUi(this);

        QString q = query;
        for (int i = 0; i < values.count(); ++i) {
            const int pos = q.indexOf(QLatin1Char('?'));
            if (pos == -1) {
                break;
            }
            q.replace(pos, 1, values.value(QStringLiteral(":%1").arg(i)).toString());
        }
        mUi->queryLbl->setText(q);
        if (!q.startsWith(QLatin1String("SELECT"))) {
            mUi->resultsLabelLbl->setText(QStringLiteral("Affected Rows:"));
        }
        mUi->resultsLbl->setText(QString::number(resultsCount));

        if (!results.isEmpty()) {
            mUi->tableWidget->setRowCount(resultsCount);
            const auto &headers = results[0];
            mUi->tableWidget->setColumnCount(headers.count());
            for (int c = 0; c < headers.count(); ++c) {
                mUi->tableWidget->setHorizontalHeaderItem(c, new QTableWidgetItem(headers[c].toString()));
            }
            for (int r = 1; r <= resultsCount; ++r) {
                const auto &row = results[r];
                for (int c = 0; c < row.size(); ++c) {
                    mUi->tableWidget->setItem(r - 1, c, new QTableWidgetItem(row[c].toString()));
                }
            }
        }

        connect(mUi->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    }

private:
    QScopedPointer<Ui::QueryViewDialog> mUi;
};

QueryDebugger::QueryDebugger(QWidget *parent)
    : QWidget(parent)
    , mUi(new Ui::QueryDebugger)
{
    qDBusRegisterMetaType<QList<QList<QVariant>>>();
    qDBusRegisterMetaType<DbConnection>();
    qDBusRegisterMetaType<QVector<DbConnection>>();

    QString service = QStringLiteral("org.freedesktop.Akonadi");
    if (Akonadi::ServerManager::hasInstanceIdentifier()) {
        service += QLatin1Char('.') + Akonadi::ServerManager::instanceIdentifier();
    }
    mDebugger = new org::freedesktop::Akonadi::StorageDebugger(service, QStringLiteral("/storageDebug"), QDBusConnection::sessionBus(), this);

    connect(mDebugger, &OrgFreedesktopAkonadiStorageDebuggerInterface::queryExecuted, this, &QueryDebugger::addQuery);

    mUi->setupUi(this);
    connect(mUi->enableDebuggingChkBox, &QAbstractButton::toggled, this, &QueryDebugger::debuggerToggled);

    mQueryList = new QueryDebuggerModel(this);
    auto proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(mQueryList);
    proxy->setDynamicSortFilter(true);
    mUi->queryListView->setModel(proxy);
    mUi->queryListView->header()->setSectionResizeMode(QueryDebuggerModel::CallsColumn, QHeaderView::Fixed);
    mUi->queryListView->header()->setSectionResizeMode(QueryDebuggerModel::DurationColumn, QHeaderView::Fixed);
    mUi->queryListView->header()->setSectionResizeMode(QueryDebuggerModel::AvgDurationColumn, QHeaderView::Fixed);
    mUi->queryListView->header()->setSectionResizeMode(QueryDebuggerModel::QueryColumn, QHeaderView::ResizeToContents);

    connect(mUi->queryTreeView, &QTreeView::doubleClicked, this, &QueryDebugger::queryTreeDoubleClicked);
    connect(mUi->saveToFileBtn, &QPushButton::clicked, this, &QueryDebugger::saveTreeToFile);
    mQueryTree = new QueryTreeModel(this);
    mUi->queryTreeView->setModel(mQueryTree);
    connect(mDebugger, &org::freedesktop::Akonadi::StorageDebugger::connectionOpened, mQueryTree, &QueryTreeModel::addConnection);
    connect(mDebugger, &org::freedesktop::Akonadi::StorageDebugger::connectionChanged, mQueryTree, &QueryTreeModel::updateConnection);
    connect(mDebugger, &org::freedesktop::Akonadi::StorageDebugger::transactionStarted, mQueryTree, &QueryTreeModel::addTransaction);
    connect(mDebugger, &org::freedesktop::Akonadi::StorageDebugger::transactionFinished, mQueryTree, &QueryTreeModel::closeTransaction);

    Akonadi::ControlGui::widgetNeedsAkonadi(this);
}

QueryDebugger::~QueryDebugger()
{
    // Disable debugging when turning off Akonadi Console so that we don't waste
    // resources on server
    mDebugger->enableSQLDebugging(false);
}

void QueryDebugger::clear()
{
    mQueryList->clear();
}

void QueryDebugger::debuggerToggled(bool on)
{
    mDebugger->enableSQLDebugging(on);
    if (on) {
        mQueryTree->clear();

        const QVector<DbConnection> conns = mDebugger->connections();
        for (const auto &con : conns) {
            mQueryTree->addConnection(con.id, con.name, con.start);
            if (con.transactionStart > 0) {
                mQueryTree->addTransaction(con.id, con.trxName, con.transactionStart, 0, QString());
            }
        }
    }
}

void QueryDebugger::addQuery(double sequence,
                             qint64 connectionId,
                             qint64 timestamp,
                             uint duration,
                             const QString &query,
                             const QMap<QString, QVariant> &values,
                             int resultsCount,
                             const QList<QList<QVariant>> &result,
                             const QString &error)
{
    Q_UNUSED(sequence)
    mQueryList->addQuery(query, duration);
    mQueryTree->addQuery(connectionId, timestamp, duration, query, values, resultsCount, result, error);
}

void QueryDebugger::queryTreeDoubleClicked(const QModelIndex &index)
{
    if (static_cast<QueryTreeModel::RowType>(index.data(QueryTreeModel::RowTypeRole).toInt()) != QueryTreeModel::Query) {
        return;
    }

    auto dlg = new QueryViewDialog(index.data(QueryTreeModel::QueryRole).toString(),
                                   index.data(QueryTreeModel::QueryValuesRole).value<QMap<QString, QVariant>>(),
                                   index.data(QueryTreeModel::QueryResultsCountRole).toInt(),
                                   index.data(QueryTreeModel::QueryResultsRole).value<QList<QList<QVariant>>>(),
                                   this);
    connect(dlg, &QDialog::finished, dlg, &QObject::deleteLater);
    dlg->show();
}

void QueryDebugger::saveTreeToFile()
{
    const QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        // show error
        return;
    }

    mQueryTree->dumpRow(file, QModelIndex(), 0);

    file.close();
}

#include "querydebugger.moc"
