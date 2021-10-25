/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <ui_browserwidget_contentview.h>
#include <ui_browserwidget_itemview.h>

#include <Akonadi/Collection>
#include <Akonadi/ETMViewStateSaver>
#include <Akonadi/Item>

#include <QWidget>

class QModelIndex;
class QStandardItemModel;

class KJob;
class KToggleAction;
class KXmlGuiWindow;

class AkonadiBrowserModel;

template<typename T> class KViewStateMaintainer;

namespace Akonadi
{
class ChangeRecorder;
class EntityTreeView;
class Job;
class StandardActionManager;
class Monitor;
class TagModel;
class StatisticsProxyModel;
class ContactViewer;
class ContactGroupViewer;
}

namespace CalendarSupport
{
class IncidenceViewer;
}

namespace MessageViewer
{
class Viewer;
}

class BrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BrowserWidget(KXmlGuiWindow *xmlGuiWindow, QWidget *parent = nullptr);
    ~BrowserWidget() override;

public Q_SLOTS:
    void dumpToXml();
    void clearCache();

private Q_SLOTS:
    void currentChanged(const QModelIndex &index);
    void itemFetchDone(KJob *job);
    void modelChanged();
    void save();
    void saveResult(KJob *job);
    void addAttribute();
    void delAttribute();
    void setItem(const Akonadi::Item &item);
    void dumpToXmlResult(KJob *job);
    void clear();
    void updateItemFetchScope();

    void contentViewChanged();

    void tagViewContextMenuRequested(const QPoint &pos);
    void tagViewDoubleClicked(const QModelIndex &index);
    void addSubTagRequested();
    void addTagRequested();
    void editTagRequested();
    void removeTagRequested();
    void createTag();
    void modifyTag();

private:
    Akonadi::Collection currentCollection() const;

    Akonadi::ChangeRecorder *mBrowserMonitor = nullptr;
    AkonadiBrowserModel *mBrowserModel = nullptr;
    Akonadi::EntityTreeView *mCollectionView = nullptr;
    Akonadi::StatisticsProxyModel *statisticsProxyModel = nullptr;
    Ui::ItemViewWidget itemUi;
    Ui::ContentViewWidget contentUi;
    Akonadi::Item mCurrentItem;
    QStandardItemModel *mAttrModel = nullptr;
    Akonadi::StandardActionManager *mStdActionManager = nullptr;
    Akonadi::Monitor *mMonitor = nullptr;
    KViewStateMaintainer<Akonadi::ETMViewStateSaver> *m_stateMaintainer = nullptr;
    KToggleAction *mCacheOnlyAction = nullptr;
    QTreeView *mTagView = nullptr;
    Akonadi::TagModel *mTagModel = nullptr;

    Akonadi::ContactViewer *mContactView = nullptr;
    Akonadi::ContactGroupViewer *mContactGroupView = nullptr;
    CalendarSupport::IncidenceViewer *mIncidenceView = nullptr;
    MessageViewer::Viewer *mMailView = nullptr;
};

