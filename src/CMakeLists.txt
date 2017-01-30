
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

add_subdirectory(icons)
add_subdirectory(kconf_update)

set(akonadiconsole_lib_tracker_SRCS
    jobtracker.cpp
    jobtrackerwidget.cpp
    jobtrackermodel.cpp
    jobtrackerfilterproxymodel.cpp
    jobtrackersearchwidget.cpp
    )

set(akonadiconsole_lib_SRCS
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
    ${akonadiconsole_lib_tracker_SRCS}
    )

qt5_generate_dbus_interface(jobtracker.h org.freedesktop.Akonadi.JobTracker.xml)
qt5_add_dbus_adaptor(akonadiconsole_lib_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.JobTracker.xml jobtracker.h JobTracker)

qt5_add_dbus_interfaces(akonadiconsole_lib_SRCS
    ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.TracerNotification.xml
    )
# Use intalled interface once we can depend on Akoandi 1.1
set_source_files_properties(org.freedesktop.Akonadi.DebugInterface.xml PROPERTIES CLASSNAME DebugInterface)
qt5_add_dbus_interface(akonadiconsole_lib_SRCS
    org.freedesktop.Akonadi.DebugInterface.xml
    debuginterface
    )

set_source_files_properties(${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.StorageDebugger.xml
    PROPERTIES INCLUDE querydebugger.h
)
qt5_add_dbus_interface(akonadiconsole_lib_SRCS
    ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.StorageDebugger.xml
    storagedebuggerinterface
    )

ki18n_wrap_ui(akonadiconsole_lib_SRCS
    agentwidget.ui
    agentconfigdialog.ui
    browserwidget_itemview.ui
    collectionattributespage.ui
    collectionaclpage.ui
    dbbrowser.ui
    dbconsole.ui
    instanceselector.ui
    browserwidget_contentview.ui
    collectioninternalspage.ui
    tagpropertiesdialog.ui
    querydebugger.ui
    queryviewdialog.ui
    )



ecm_qt_declare_logging_category(akonadiconsole_lib_SRCS HEADER akonadiconsole_debug.h IDENTIFIER AKONADICONSOLE_LOG CATEGORY_NAME org.kde.pim.akonadiconsole)
add_library(akonadiconsolelib ${akonadiconsole_lib_SRCS})

generate_export_header(akonadiconsolelib BASE_NAME akonadiconsolelib)

target_link_libraries(akonadiconsolelib
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

set_target_properties(akonadiconsolelib
    PROPERTIES VERSION ${KDEPIM_LIB_VERSION} SOVERSION ${KDEPIM_LIB_SOVERSION}
    )



set(akonadiconsole_bin_SRCS
    main.cpp
    )

qt5_add_resources(akonadiconsole_bin_SRCS akonadiconsole.qrc)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-akonadiconsole.png")
ecm_add_app_icon(akonadiconsole_bin_SRCS ICONS ${ICONS_SRCS})

add_executable(akonadiconsole_bin ${akonadiconsole_bin_SRCS})
set_target_properties(akonadiconsole_bin PROPERTIES OUTPUT_NAME akonadiconsole)

target_link_libraries(akonadiconsole_bin
    akonadiconsolelib
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

install(TARGETS akonadiconsole_bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.akonadiconsole.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS akonadiconsolelib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
