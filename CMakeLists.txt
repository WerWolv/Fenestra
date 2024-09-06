cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)

set(FENESTRA_APPLICATION_NAME "Fenestra" CACHE STRING "Application name")
set(FENESTRA_APPLICATION_AUTHOR "Author" CACHE STRING "Application author")
set(FENESTRA_MAIN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" CACHE STRING "Main output directory")
set(FENESTRA_APPLICATION_PLUGIN_DIRECTORY "" CACHE STRING "Application plugin directory")
set(FENESTRA_STRICT_WARNINGS ON CACHE BOOL "Enable strict warnings")


set(FENESTRA_BASE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}")
set(FENESTRA_APPLICATION_BASE_FOLDER "${CMAKE_SOURCE_DIR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(cmake/build_helpers.cmake)

loadVersion(FENESTRA_APPLICATION_VERSION FENESTRA_APPLICATION_VERSION_PLAIN)
setVariableInParent(FENESTRA_APPLICATION_VERSION ${FENESTRA_APPLICATION_VERSION})

project(fenestra
        LANGUAGES       C CXX
        VERSION         ${FENESTRA_APPLICATION_VERSION_PLAIN}
        DESCRIPTION     "The Fenestra UI base system"
        HOMEPAGE_URL    "https://werwolv.net"
)

add_custom_target(fenestra_all)

configureCMake()
addDefines()
detectOS()
addBundledLibraries()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_subdirectory(lib)
add_subdirectory(gui)
if (WIN32)
    add_subdirectory(forwarder)
endif ()

detectBundledPlugins()
addPluginDirectories()