/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADICONSOLE_LOGGINGMODEL_H_
#define AKONADICONSOLE_LOGGINGMODEL_H_

#include <QAbstractItemModel>
#include <QSet>

class QStandardItemModel;

class LoggingModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    struct Message {
        qint64 timestamp;
        QString app;
        qint64 pid;
        QString category;
        QString file;
        QString function;
        QString message;
        QtMsgType type;
        int line;
    };

    enum Roles { MessageRole = Qt::UserRole };

    enum Columns {
        TimeColumn,
        AppColumn,
        TypeColumn,
        CategoryColumn,
        FileColumn,
        FunctionColumn,
        MessageColumn,

        _ColumnCount
    };

    explicit LoggingModel(QObject *parent = nullptr);
    ~LoggingModel() override;

    void addMessage(qint64 timestamp,
                    const QString &app,
                    qint64 pid,
                    QtMsgType type,
                    const QString &category,
                    const QString &file,
                    const QString &function,
                    int line,
                    const QString &message);

    void setAppFilterModel(QStandardItemModel *appFilterModel);
    void setCategoryFilterModel(QStandardItemModel *categoryFilterModel);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child = {}) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    static QString cacheString(const QString &str, QSet<QString> &cache, QStandardItemModel *model = nullptr);

    QVector<Message> mMessages;
    QStandardItemModel *mAppFilterModel = nullptr;
    QSet<QString> mAppCache;
    QStandardItemModel *mCategoryFilterModel = nullptr;
    QSet<QString> mCategoryCache;
    QSet<QString> mFileCache;
    QSet<QString> mFunctionCache;
};

#endif
