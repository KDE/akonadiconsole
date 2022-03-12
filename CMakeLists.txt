cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "5.19.80")
project(akonadiconsole VERSION ${PIM_VERSION})

# Needs C++17 because Akonadi uses it and we link
# against its private API

set(KF5_MIN_VERSION "5.91.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(ECMInstallIcons)


include(ECMSetupVersion)
include(ECMAddTests)

include(GenerateExportHeader)
include(ECMGenerateHeaders)

include(FeatureSummary)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
include(CheckFunctionExists)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)


# Do NOT add quote
set(KDEPIM_DEV_VERSION alpha)

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()
set(RELEASE_SERVICE_VERSION "22.03.80")

set(KDEPIM_VERSION "${PIM_VERSION}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")

set(KDEPIM_LIB_VERSION "${PIM_VERSION}")
set(KDEPIM_LIB_SOVERSION "5")

set(AKONADI_CONTACT_VERSION "5.19.80")
set(CALENDARSUPPORT_LIB_VERSION "5.19.80")
set(AKONADI_VERSION "5.19.80")
set(AKONADI_SEARCH_VERSION "5.19.80")

set(QT_REQUIRED_VERSION "5.15.2")
find_package(Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets DBus Sql Test)
set(MESSAGELIB_LIB_VERSION "5.19.80")
set(LIBKDEPIM_LIB_VERSION "5.19.80")
set(KMIME_LIB_VERSION "5.19.80")


# Find KF5 package
find_package(KF5Completion ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DBusAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DocTools ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5TextWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5WidgetsAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5XmlGui ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Crash ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5Completion ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5ItemViews ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5KIO ${KF5_MIN_VERSION} REQUIRED)

# Find KdepimLibs Package
find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF5Contacts ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED) # Because of KPIM::KCheckComboBox

if (NOT WIN32)
    find_package(KF5AkonadiContact ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
    find_package(KF5CalendarSupport ${CALENDARSUPPORT_LIB_VERSION} CONFIG REQUIRED)
    find_package(KF5MessageViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
    find_package(KF5AkonadiSearch ${AKONADI_SEARCH_VERSION} CONFIG REQUIRED)
    set_package_properties(KF5AkonadiSearch PROPERTIES DESCRIPTION "The Akonadi Search libraries" URL "https://invent.kde.org/pim/akonadi-search" TYPE REQUIRED PURPOSE "Provides search capabilities in KMail and Akonadi")
    find_package(Xapian CONFIG)
    set_package_properties(Xapian PROPERTIES
        DESCRIPTION "The Xapian search engine library"
        URL "https://xapian.org"
        TYPE REQUIRED
    )

    set(ENABLE_SEARCH TRUE)
    set(ENABLE_CONTENTVIEWS TRUE)
endif()

# From akonadi's CMakeLists.txt. Needed to fix:
# FAILED: src/CMakeFiles/libakonadiconsole.dir/browserwidget.cpp.obj
# ...
# Z:\CraftRoot\include\KF5\AkonadiCore/item.h(596): error C2039:
# "auto_ptr" ist kein Member von "std".
if (MSVC)
    # This sets the __cplusplus macro to a real value based on the version of C++ specified by
    # the /std switch. Without it MSVC keeps reporting C++98, so feature detection doesn't work.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

ecm_set_disabled_deprecation_versions(QT 5.15.2 KF 5.92.0)


include_directories(${akonadiconsole_SOURCE_DIR} ${akonadiconsole_BINARY_DIR} ${XAPIAN_INCLUDE_DIR})
configure_file(akonadiconsole-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadiconsole-version.h @ONLY)
configure_file(config-akonadiconsole.h.cmake ${CMAKE_BINARY_DIR}/config-akonadiconsole.h)
option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if (USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
    add_definitions(-DCOMPILE_WITH_UNITY_CMAKE_SUPPORT)
endif()
add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
