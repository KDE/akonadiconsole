/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "loggingmodel.h"

#include <QDateTime>
#include <QDir>
#include <QStandardItemModel>

Q_DECLARE_METATYPE(LoggingModel::Message)

LoggingModel::LoggingModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

LoggingModel::~LoggingModel()
{
}

QString LoggingModel::cacheString(const QString &str, QSet<QString> &cache, QStandardItemModel *model)
{
    auto it = cache.constFind(str);
    if (it == cache.constEnd()) {
        if (model) {
            auto item = new QStandardItem(str);
            item->setCheckState(Qt::Checked);
            model->appendRow(item);
        }
        cache.insert(str);
        return str;
    }
    return *it;
}

void LoggingModel::addMessage(qint64 timestamp, const QString &app, qint64 pid, QtMsgType type, const QString &category, const QString &file, const QString &function, int line, const QString &message)
{
    beginInsertRows({}, mMessages.count(), mMessages.count());
    mMessages.push_back({ timestamp, cacheString(app, mAppCache, mAppFilterModel), pid,
                          cacheString(category, mCategoryCache, mCategoryFilterModel),
                          cacheString(file, mFileCache), cacheString(function, mFunctionCache),
                          message, type, line });
    endInsertRows();
}

void LoggingModel::setAppFilterModel(QStandardItemModel *appFilterModel)
{
    mAppFilterModel = appFilterModel;
}

void LoggingModel::setCategoryFilterModel(QStandardItemModel *categoryFilterModel)
{
    mCategoryFilterModel = categoryFilterModel;
}

int LoggingModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mMessages.count();
}

int LoggingModel::columnCount(const QModelIndex &) const
{
    return _ColumnCount;
}

QModelIndex LoggingModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= mMessages.count() || column < 0 || column >= _ColumnCount) {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex LoggingModel::parent(const QModelIndex &) const
{
    return {};
}

QVariant LoggingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case TimeColumn:
        return QStringLiteral("Time");
    case AppColumn:
        return QStringLiteral("Program");
    case TypeColumn:
        return QStringLiteral("Type");
    case CategoryColumn:
        return QStringLiteral("Category");
    case FileColumn:
        return QStringLiteral("File");
    case FunctionColumn:
        return QStringLiteral("Function");
    case MessageColumn:
        return QStringLiteral("Message");
    }
    return {};
}

QVariant LoggingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mMessages.count() || index.column() >= _ColumnCount) {
        return {};
    }

    const auto message = mMessages.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case TimeColumn:
            return QDateTime::fromMSecsSinceEpoch(message.timestamp).toString(Qt::ISODateWithMs);
        case AppColumn:
            return QStringLiteral("%1(%2)").arg(message.app, QString::number(message.pid));
        case TypeColumn:
            switch (message.type) {
            case QtDebugMsg:
                return QStringLiteral("Debug");
            case QtInfoMsg:
                return QStringLiteral("Info");
            case QtWarningMsg:
                return QStringLiteral("Warning");
            case QtCriticalMsg:
                return QStringLiteral("Critical");
            case QtFatalMsg:
                return QStringLiteral("Fatal");
            }
            return {};
        case CategoryColumn:
            return message.category;
        case FileColumn:
            if (!message.file.isEmpty()) {
                const auto file = message.file.mid(message.file.lastIndexOf(QDir::separator()) + 1, -1);
                if (message.line > 0) {
                    return QStringLiteral("%1:%2").arg(file, QString::number(message.line));
                } else {
                    return file;
                }
            }
            return {};
        case FunctionColumn:
            return message.function;
        case MessageColumn:
            return message.message;
        }
    } else if (role == Qt::ToolTipRole) {
        switch (index.column()) {
        case FileColumn:
            if (!message.file.isEmpty()) {
                if (message.line > 0) {
                    return QStringLiteral("%1:%2").arg(message.file, QString::number(message.line));
                } else {
                    return message.file;
                }
            }
            return {};
        case FunctionColumn:
            return message.function;
        case MessageColumn:
            return message.message;
        }
    } else if (role == MessageRole) {
        return QVariant::fromValue(message);
    }

    return {};
}
