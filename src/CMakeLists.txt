
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_CAST_TO_ASCII)

add_subdirectory(icons)
add_subdirectory(kconf_update)

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
    dbaccess.cpp
    dbbrowser.cpp
    dbconsole.cpp
    debugwidget.cpp
    instanceselector.cpp
    mainwidget.cpp
    mainwindow.cpp
    monitorswidget.cpp
    monitorsmodel.cpp
    notificationmodel.cpp
    notificationmonitor.cpp
    searchdialog.cpp
    searchwidget.cpp
    querydebugger.cpp
    tagpropertiesdialog.cpp
    ${libakonadiconsole_tracker_SRCS}
    )

qt5_generate_dbus_interface(jobtracker.h org.freedesktop.Akonadi.JobTracker.xml)
qt5_add_dbus_adaptor(libakonadiconsole_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.JobTracker.xml jobtracker.h JobTracker)

qt5_add_dbus_interfaces(libakonadiconsole_SRCS
    ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.TracerNotification.xml
    )
# Use intalled interface once we can depend on Akoandi 1.1
set_source_files_properties(org.freedesktop.Akonadi.DebugInterface.xml PROPERTIES CLASSNAME DebugInterface)
qt5_add_dbus_interface(libakonadiconsole_SRCS
    org.freedesktop.Akonadi.DebugInterface.xml
    debuginterface
    )

set_source_files_properties(${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.StorageDebugger.xml
    PROPERTIES INCLUDE querydebugger.h
)
qt5_add_dbus_interface(libakonadiconsole_SRCS
    ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.StorageDebugger.xml
    storagedebuggerinterface
    )

qt5_wrap_ui(libakonadiconsole_SRCS
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



ecm_qt_declare_logging_category(libakonadiconsole_SRCS HEADER akonadiconsole_debug.h IDENTIFIER AKONADICONSOLE_LOG CATEGORY_NAME org.kde.pim.akonadiconsole)
add_library(libakonadiconsole ${libakonadiconsole_SRCS})

generate_export_header(libakonadiconsole BASE_NAME libakonadiconsole)

target_link_libraries(libakonadiconsole
    KF5::AkonadiCore
    KF5::AkonadiPrivate
    KF5::AkonadiContact
    KF5::AkonadiWidgets
    KF5::AkonadiXml
    KF5::CalendarSupport
    KF5::CalendarCore
    KF5::Libkdepim
    KF5::MessageViewer
    KF5::DBusAddons
    KF5::PimTextEdit
    Qt5::Sql
    KF5::Crash
    )

set_target_properties(libakonadiconsole
    PROPERTIES OUTPUT_NAME akonadiconsole
    VERSION ${KDEPIM_LIB_VERSION}
    SOVERSION ${KDEPIM_LIB_SOVERSION}
    )


set(akonadiconsole_SRCS
    main.cpp
    )

qt5_add_resources(akonadiconsole_SRCS akonadiconsole.qrc)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-akonadiconsole.png")
ecm_add_app_icon(akonadiconsole_SRCS ICONS ${ICONS_SRCS})

add_executable(akonadiconsole ${akonadiconsole_SRCS})

target_link_libraries(akonadiconsole
    libakonadiconsole
    KF5::AkonadiCore
    KF5::AkonadiPrivate
    KF5::AkonadiContact
    KF5::AkonadiXml
    KF5::CalendarSupport
    KF5::CalendarCore
    KF5::Libkdepim
    KF5::MessageViewer
    KF5::DBusAddons
    KF5::PimTextEdit
    Qt5::Sql
    KF5::Crash
    )

install(TARGETS akonadiconsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.akonadiconsole.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS libakonadiconsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
