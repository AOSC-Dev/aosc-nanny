cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0048 NEW)

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()
if (POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()

project(aosc-nanny VERSION 0.0.0)

set(QT_MIN_VERSION "5.8.0")
set(KF_MIN_VERSION "5.96.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.85.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH
    ${ECM_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(ECMSetupVersion)
include(FeatureSummary)
include(GenerateExportHeader)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(KDEGitCommitHooks)
include(KDEClangFormat)

if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(QT_MIN_VERSION "${QT_REQUIRED_VERSION}")
    set(KF_MIN_VERSION "5.240.0")
    set(KF_MAJOR_VERSION "6")
else()
    set(KF_MAJOR_VERSION "5")
endif()

set(REQUIRED_QT_VERSION "${QT_MIN_VERSION}") # Used in CMakeConfig to check deps

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED
    COMPONENTS
        Widgets
)

find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED
    COMPONENTS
        CoreAddons
        I18n
        IconThemes
)

if(ECM_VERSION VERSION_GREATER 1.5.0)
    # >1.5 has an optional compatibility option for the package version file.
    # TODO: bump dep and remove this check once 1.7 is out.
    set(VERSION_COMPAT_OPTION COMPATIBILITY SameMajorVersion)
endif()

add_definitions(-DPROJECT_VERSION=\"${PROJECT_VERSION}\")
add_definitions(
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050900
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(src)

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
