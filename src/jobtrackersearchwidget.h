/*
  SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef JOBTRACKERSEARCHWIDGET_H
#define JOBTRACKERSEARCHWIDGET_H

#include <QWidget>

#include "libakonadiconsole_export.h"

class QLineEdit;
class QComboBox;
class QCheckBox;
class LIBAKONADICONSOLE_EXPORT JobTrackerSearchWidget : public QWidget
{
    Q_OBJECT
public:
    explicit JobTrackerSearchWidget(QWidget *parent = nullptr);
    ~JobTrackerSearchWidget();

Q_SIGNALS:
    void searchTextChanged(const QString &);
    void columnChanged(int col);
    void selectOnlyErrorChanged(bool state);

private Q_SLOTS:
    void slotColumnChanged(int index);

private:
    QLineEdit *mSearchLineEdit = nullptr;
    QComboBox *mSelectColumn = nullptr;
    QCheckBox *mSelectOnlyError = nullptr;
};

#endif // JOBTRACKERSEARCHWIDGET_H
