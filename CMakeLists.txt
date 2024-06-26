cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project(sdk_launcher
        DESCRIPTION "A minimal SDK launcher for Strata Source engine games"
        VERSION "0.2.0"
        HOMEPAGE_URL "https://github.com/StrataSource/sdk-launcher")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include CMake libraries
include(CheckIPOSupported)
include(GNUInstallDirs)

# Set up variables
set(PROJECT_NAME_PRETTY "SDK Launcher" CACHE STRING "" FORCE)
set(PROJECT_TARGET_NAME "SDKLauncher"  CACHE STRING "" FORCE)

# Helper for creating enum options (pick between certain strings)
macro(option_enum)
    cmake_parse_arguments(IN "" "NAME;DEFAULT;DESCRIPTION" "OPTIONS" ${ARGN})
    set(${IN_NAME}_OPTIONS ${IN_OPTIONS})
    set(${IN_NAME} ${IN_DEFAULT} CACHE STRING ${IN_DESCRIPTION})
    set_property(CACHE ${IN_NAME} PROPERTY STRINGS ${IN_NAME}_OPTIONS)
    if(NOT ${IN_NAME} IN_LIST ${IN_NAME}_OPTIONS)
        message(FATAL_ERROR "${IN_NAME} (currently ${${IN_NAME}}) must be one of the following values: ${${IN_NAME}_OPTIONS}")
    endif()
endmacro()

# Options
option(SDK_LAUNCHER_USE_LTO "Build SDK Launcher with link-time optimization enabled" OFF)
option_enum(
        NAME "SDK_LAUNCHER_DEFAULT_MOD"
        DESCRIPTION "The default game folder to use"
        DEFAULT "p2ce"
        OPTIONS "p2ce" "revolution" "momentum")

# Global CMake options
if(PROJECT_IS_TOP_LEVEL)
    # Set proper runpath
    set(CMAKE_SKIP_BUILD_RPATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH $ORIGIN)

    # Compile with PIC
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # Compile with LTO if supported
    set(SDK_LAUNCHER_USE_LTO_INTERNAL OFF)
    if(SDK_LAUNCHER_USE_LTO)
        check_ipo_supported(RESULT SDK_LAUNCHER_USE_LTO_INTERNAL)
    endif()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${SDK_LAUNCHER_USE_LTO_INTERNAL})

    # Set default install directory permissions
    set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# Macro to set up many things at once for a given target
function(sdk_launcher_configure_target TARGET)
    # Define DEBUG macro
    target_compile_definitions(${TARGET} PRIVATE "$<$<CONFIG:Debug>:DEBUG>")

    # Set optimization flags
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        # Build with debug friendly optimizations and debug symbols (MSVC defaults are fine)
        if(UNIX OR MINGW)
            target_compile_options(${TARGET} PRIVATE -Og -g)
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
            target_compile_options(${TARGET} PRIVATE -fno-limit-debug-info)
        endif()
    else()
        # Build with optimizations and don't omit stack pointer for debugging (MSVC defaults are fine)
        if(UNIX OR MINGW)
            target_compile_options(${TARGET} PRIVATE -O2 -fno-omit-frame-pointer)
        endif()
    endif()

    if(WIN32 AND MSVC)
        get_target_property(TARGET_TYPE ${TARGET} TYPE)
        get_target_property(TARGET_IS_GUI ${TARGET} WIN32_EXECUTABLE)

        # Create PDBs in release
        if(MSVC AND ((TARGET_TYPE STREQUAL "SHARED_LIBRARY") OR (TARGET_TYPE STREQUAL "EXECUTABLE")))
            target_compile_options(
                    ${TARGET} PRIVATE
                    "$<$<CONFIG:Release>:/Zi>")
            target_link_options(
                    ${TARGET} PRIVATE
                    "$<$<CONFIG:Release>:/DEBUG>"
                    "$<$<CONFIG:Release>:/OPT:REF>"
                    "$<$<CONFIG:Release>:/OPT:ICF>")
        endif()

        # Don't show the console when running the executable
        if(MSVC AND TARGET_IS_GUI)
            target_link_options(
                    ${TARGET} PRIVATE
                    "/ENTRY:mainCRTStartup")
        endif()
    endif()
endfunction()

# Qt
if(WIN32 AND NOT DEFINED QT_BASEDIR)
    message(FATAL_ERROR "Please define your QT install dir with -DQT_BASEDIR=\"C:/your/qt6/here\"")
endif()

if(DEFINED QT_BASEDIR)
    string(REPLACE "\\" "/" QT_BASEDIR "${QT_BASEDIR}")

    # Add it to the prefix path so find_package can find it
    list(APPEND CMAKE_PREFIX_PATH "${QT_BASEDIR}")
    set(QT_INCLUDE "${QT_BASEDIR}/include")
    message(STATUS "Using ${QT_INCLUDE} as the Qt include directory")
endif()

# CMake has an odd policy that links a special link lib for Qt on newer versions of CMake
cmake_policy(SET CMP0020 NEW)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Generate config header
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h")

# Create executable
add_executable(${PROJECT_TARGET_NAME} WIN32
        "${CMAKE_CURRENT_SOURCE_DIR}/res/res.qrc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GameConfig.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GameConfig.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Window.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Window.h")

sdk_launcher_configure_target(${PROJECT_TARGET_NAME})

target_link_libraries(
        ${PROJECT_TARGET_NAME} PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets)

target_include_directories(
        ${PROJECT_TARGET_NAME} PRIVATE
        "${QT_INCLUDE}"
        "${QT_INCLUDE}/QtCore"
        "${QT_INCLUDE}/QtGui"
        "${QT_INCLUDE}/QtWidgets")
