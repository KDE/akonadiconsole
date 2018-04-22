/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

    enum Roles {
        MessageRole = Qt::UserRole
    };

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

    void addMessage(qint64 timestamp, const QString &app, qint64 pid, QtMsgType type,
                    const QString &category, const QString &file, const QString &function,
                    int line, const QString &message);

    void setAppFilterModel(QStandardItemModel *appFilterModel);
    void setCategoryFilterModel(QStandardItemModel *categoryFilterModel);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child = {}) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    QString cacheString(const QString &str, QSet<QString> &cache, QStandardItemModel *model = nullptr);

    QVector<Message> mMessages;
    QStandardItemModel *mAppFilterModel = nullptr;
    QSet<QString> mAppCache;
    QStandardItemModel *mCategoryFilterModel = nullptr;
    QSet<QString> mCategoryCache;
    QSet<QString> mFileCache;
    QSet<QString> mFunctionCache;
};

#endif
