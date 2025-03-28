/*
  SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

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
    ~JobTrackerSearchWidget() override;

Q_SIGNALS:
    void searchTextChanged(const QString &);
    void columnChanged(int col);
    void selectOnlyErrorChanged(bool state);

private:
    void slotColumnChanged(int index);
    QLineEdit *const mSearchLineEdit;
    QComboBox *const mSelectColumn;
    QCheckBox *const mSelectOnlyError;
};
