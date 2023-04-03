
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DTRANSLATION_DOMAIN=\"akonadiconsole\")

add_subdirectory(icons)

set(libakonadiconsole_tracker_SRCS
    jobtracker.cpp
    jobtrackerwidget.cpp
    jobtrackermodel.cpp
    jobtrackerfilterproxymodel.cpp
    jobtrackersearchwidget.cpp
    )

set(libakonadiconsole_SRCS
    agentwidget.cpp
    agentconfigdialog.cpp
    agentconfigmodel.cpp
    akonadibrowsermodel.cpp
    browserwidget.cpp
    collectionattributespage.cpp
    collectioninternalspage.cpp
    collectionaclpage.cpp
    connectionpage.cpp
    dbbrowser.cpp
    dbconsole.cpp
    debugfiltermodel.cpp
    debugmodel.cpp
    debugwidget.cpp
    instanceselector.cpp
    logging.cpp
    loggingfiltermodel.cpp
    loggingmodel.cpp
    mainwidget.cpp
    mainwindow.cpp
    monitorswidget.cpp
    monitorsmodel.cpp
    notificationmodel.cpp
    notificationfiltermodel.cpp
    notificationmonitor.cpp
    querydebugger.cpp
    tagpropertiesdialog.cpp
    uistatesaver.cpp
    monitorsmodel.h
    notificationfiltermodel.h
    collectionaclpage.h
    utils.h
    jobtrackermodel.h
    metatypes.h
    jobtrackersearchwidget.h
    debugwidget.h
    jobtracker.h
    collectioninternalspage.h
    mainwindow.h
    browserwidget.h
    akonadibrowsermodel.h
    collectionattributespage.h
    notificationmonitor.h
    agentconfigmodel.h
    monitorswidget.h
    notificationmodel.h
    mainwidget.h
    dbconsole.h
    tagpropertiesdialog.h
    querydebugger.h
    logging.h
    uistatesaver.h
    debugfiltermodel.h
    loggingfiltermodel.h
    dbbrowser.h
    agentconfigdialog.h
    jobtrackerwidget.h
    connectionpage.h
    jobtrackerfilterproxymodel.h
    loggingmodel.h
    agentwidget.h
    debugmodel.h
    instanceselector.h
    ${libakonadiconsole_tracker_SRCS}
)

if (ENABLE_SEARCH)
    set(libakonadiconsole_SRCS
        ${libakonadiconsole_SRCS}
        searchwidget.cpp
        searchwidget.h
    )
endif()

qt_generate_dbus_interface(jobtracker.h org.freedesktop.Akonadi.JobTracker.xml)
qt_add_dbus_adaptor(libakonadiconsole_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.JobTracker.xml jobtracker.h JobTracker)
qt_add_dbus_adaptor(libakonadiconsole_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.akonadiconsole.logger.xml logging.h Logging)

qt_add_dbus_interfaces(libakonadiconsole_SRCS
    ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.TracerNotification.xml
    )
# Use intalled interface once we can depend on Akoandi 1.1
set_source_files_properties(org.freedesktop.Akonadi.DebugInterface.xml PROPERTIES CLASSNAME DebugInterface)
qt_add_dbus_interface(libakonadiconsole_SRCS
    org.freedesktop.Akonadi.DebugInterface.xml
    debuginterface
    )

set_source_files_properties(${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.StorageDebugger.xml
    PROPERTIES INCLUDE querydebugger.h
)
qt_add_dbus_interface(libakonadiconsole_SRCS
    ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.StorageDebugger.xml
    storagedebuggerinterface
    )

qt_wrap_ui(libakonadiconsole_SRCS
    agentwidget.ui
    agentconfigdialog.ui
    browserwidget_itemview.ui
    collectionattributespage.ui
    collectionaclpage.ui
    dbbrowser.ui
    dbconsoletab.ui
    instanceselector.ui
    browserwidget_contentview.ui
    collectioninternalspage.ui
    tagpropertiesdialog.ui
    querydebugger.ui
    queryviewdialog.ui
    )


add_library(libakonadiconsole ${libakonadiconsole_SRCS})

ecm_qt_declare_logging_category(libakonadiconsole HEADER akonadiconsole_debug.h IDENTIFIER AKONADICONSOLE_LOG CATEGORY_NAME org.kde.pim.akonadiconsole
        OLD_CATEGORY_NAMES log_akonadiconsole
        DESCRIPTION "akonadiconsole (akonadiconsole)" EXPORT AKONADICONSOLE)



if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(libakonadiconsole PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(libakonadiconsole BASE_NAME libakonadiconsole)

target_link_libraries(libakonadiconsole
    KPim${KF_MAJOR_VERSION}::AkonadiCore
    KPim${KF_MAJOR_VERSION}::AkonadiPrivate
    KPim${KF_MAJOR_VERSION}::AkonadiWidgets
    KPim${KF_MAJOR_VERSION}::AkonadiXml
    KPim${KF_MAJOR_VERSION}::Mime
    KF${KF_MAJOR_VERSION}::Contacts
    KF${KF_MAJOR_VERSION}::CalendarCore
    Qt::Sql
    KF${KF_MAJOR_VERSION}::Completion
    KF${KF_MAJOR_VERSION}::ItemViews
    KF${KF_MAJOR_VERSION}::TextWidgets
    KF${KF_MAJOR_VERSION}::XmlGui
    KPim${KF_MAJOR_VERSION}::Libkdepim
    )

if (ENABLE_SEARCH)
    target_link_libraries(libakonadiconsole
        KPim${KF_MAJOR_VERSION}::AkonadiSearchCore
        KPim${KF_MAJOR_VERSION}::AkonadiSearchXapian
    )
endif()

if (ENABLE_CONTENTVIEWS)
    target_link_libraries(libakonadiconsole
        KPim${KF_MAJOR_VERSION}::AkonadiContact
        KPim${KF_MAJOR_VERSION}::ContactEditor
        KPim${KF_MAJOR_VERSION}::CalendarSupport
        KPim${KF_MAJOR_VERSION}::MessageViewer
    )
endif()

set_target_properties(libakonadiconsole
    PROPERTIES OUTPUT_NAME akonadiconsole
    VERSION ${KDEPIM_LIB_VERSION}
    SOVERSION ${KDEPIM_LIB_SOVERSION}
    )


set(akonadiconsole_SRCS
    main.cpp
    akonadiconsole.qrc
    )

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-akonadiconsole.png")
ecm_add_app_icon(akonadiconsole_SRCS ICONS ${ICONS_SRCS})

add_executable(akonadiconsole ${akonadiconsole_SRCS})

target_link_libraries(akonadiconsole
    libakonadiconsole
    KF${KF_MAJOR_VERSION}::DBusAddons
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::Crash
)

install(TARGETS akonadiconsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.akonadiconsole.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS libakonadiconsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

ecm_qt_install_logging_categories(EXPORT AKONADICONSOLE FILE akonadiconsole.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
