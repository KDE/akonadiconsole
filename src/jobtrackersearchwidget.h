/*
  Copyright (c) 2017 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef JOBTRACKERSEARCHWIDGET_H
#define JOBTRACKERSEARCHWIDGET_H

#include <QWidget>

#include "akonadiconsolelib_export.h"

class QLineEdit;
class QComboBox;
class QCheckBox;
class AKONADICONSOLELIB_EXPORT JobTrackerSearchWidget : public QWidget
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
    QLineEdit *mSearchLineEdit;
    QComboBox *mSelectColumn;
    QCheckBox *mSelectOnlyError;
};

#endif // JOBTRACKERSEARCHWIDGET_H
