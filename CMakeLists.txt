cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "6.3.2")
project(akonadiconsole VERSION ${PIM_VERSION})

set(KF_MIN_VERSION "6.6.0")
set(QT_REQUIRED_VERSION "6.7.0")
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(ECMInstallIcons)


include(ECMSetupVersion)
include(ECMAddTests)

include(GenerateExportHeader)
include(ECMGenerateHeaders)

include(FeatureSummary)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES
autotests/*.cpp
autotests/*.h
src/*.cpp
src/*.h
)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
include(CheckFunctionExists)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(ECMFeatureSummary)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Do NOT add quote
set(KDEPIM_DEV_VERSION )

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()
set(RELEASE_SERVICE_VERSION "24.12.2")

set(KDEPIM_VERSION "${PIM_VERSION}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")

set(KDEPIM_LIB_VERSION "${PIM_VERSION}")
set(KDEPIM_LIB_SOVERSION "6")

set(AKONADI_CONTACT_VERSION "6.3.2")
set(CALENDARSUPPORT_LIB_VERSION "6.3.2")
set(AKONADIMIME_LIB_VERSION "6.3.2")
set(AKONADI_VERSION "6.3.2")
set(AKONADI_SEARCH_VERSION "6.3.2")

find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets DBus Sql Test)
set(MESSAGELIB_LIB_VERSION "6.3.2")
set(LIBKDEPIM_LIB_VERSION "6.3.2")
set(KMIME_LIB_VERSION "6.3.2")


# Find KF6 package
find_package(KF6Completion ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Config ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6DBusAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6DocTools ${KF_MIN_VERSION} REQUIRED)
find_package(KF6I18n ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6ItemModels ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6TextWidgets ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6WidgetsAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6XmlGui ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Crash ${KF_MIN_VERSION} REQUIRED)
find_package(KF6Completion ${KF_MIN_VERSION} REQUIRED)
find_package(KF6ItemViews ${KF_MIN_VERSION} REQUIRED)
find_package(KF6KIO ${KF_MIN_VERSION} REQUIRED)

# Find KdepimLibs Package
find_package(KPim6Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF6Contacts ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6CalendarCore ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KPim6Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6AkonadiMime ${AKONADIMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED) # Because of KPIM::KCheckComboBox

if(NOT WIN32)
    find_package(KPim6AkonadiContactWidgets ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
    find_package(KPim6CalendarSupport ${CALENDARSUPPORT_LIB_VERSION} CONFIG REQUIRED)
    find_package(KPim6MessageViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
    find_package(KPim6AkonadiSearch ${AKONADI_SEARCH_VERSION} CONFIG REQUIRED)
    set_package_properties(KPim6AkonadiSearch PROPERTIES DESCRIPTION "The Akonadi Search libraries" URL "https://invent.kde.org/pim/akonadi-search" TYPE REQUIRED PURPOSE "Provides search capabilities in KMail and Akonadi")
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
# Z:\CraftRoot\include\KF6\AkonadiCore/item.h(596): error C2039:
# "auto_ptr" ist kein Member von "std".
if(MSVC)
    # This sets the __cplusplus macro to a real value based on the version of C++ specified by
    # the /std switch. Without it MSVC keeps reporting C++98, so feature detection doesn't work.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
ecm_set_disabled_deprecation_versions(QT 6.8.0 KF 6.8.0)


include_directories(${akonadiconsole_SOURCE_DIR} ${akonadiconsole_BINARY_DIR} ${XAPIAN_INCLUDE_DIR})
configure_file(akonadiconsole-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/akonadiconsole-version.h @ONLY)
configure_file(config-akonadiconsole.h.in ${CMAKE_BINARY_DIR}/config-akonadiconsole.h)
option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if(USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
    add_definitions(-DCOMPILE_WITH_UNITY_CMAKE_SUPPORT)
endif()
add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

ki18n_install(po)

ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
