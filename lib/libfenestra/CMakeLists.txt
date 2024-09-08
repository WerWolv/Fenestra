project(libfenestra)

set(CMAKE_CXX_STANDARD 23)

if (FENESTRA_STATIC_LINK_PLUGINS)
    set(LIBFENESTRA_LIBRARY_TYPE STATIC)
else()
    set(LIBFENESTRA_LIBRARY_TYPE SHARED)
endif()

add_library(libfenestra ${LIBFENESTRA_LIBRARY_TYPE}
        source/api/fenestra_manager.cpp
        source/api/event_manager.cpp
        source/api/localization_manager.cpp
        source/api/plugin_manager.cpp
        source/api/task_manager.cpp
        source/api/layout_manager.cpp
        source/api/workspace_manager.cpp
        source/api/tutorial_manager.cpp
        source/api/theme_manager.cpp
        source/api/shortcut_manager.cpp
        source/api/interface_registry.cpp

        source/helpers/logger.cpp
        source/helpers/utils.cpp
        source/helpers/utils_linux.cpp
        source/helpers/default_paths.cpp
        source/helpers/fs.cpp
        source/helpers/imgui_hooks.cpp

        source/subcommands/subcommands.cpp

        source/ui/imgui_extensions.cpp
        source/ui/popup.cpp
        source/ui/toast.cpp
        source/ui/view.cpp
)

if (DEFINED FENESTRA_COMMIT_HASH_LONG AND DEFINED FENESTRA_COMMIT_BRANCH)
    set(GIT_COMMIT_HASH_LONG "${FENESTRA_COMMIT_HASH_LONG}")
    set(GIT_BRANCH "${FENESTRA_COMMIT_BRANCH}")
else()
    # Get the current working branch
    execute_process(
            COMMAND git rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE RESULT_BRANCH
            ERROR_QUIET
    )

    execute_process(
            COMMAND git log -1 --format=%H
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH_LONG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE RESULT_HASH_LONG
            ERROR_QUIET
    )
endif ()

if (NOT DEFINED GIT_COMMIT_HASH_LONG OR GIT_COMMIT_HASH_LONG STREQUAL "" OR NOT DEFINED GIT_BRANCH OR GIT_BRANCH STREQUAL "")
    message(WARNING "Failed to to determine commit hash/branch")
else()
    addDefineToSource(source/api/fenestra_manager.cpp "GIT_COMMIT_HASH_LONG=\"${GIT_COMMIT_HASH_LONG}\"")
    addDefineToSource(source/api/fenestra_manager.cpp "GIT_BRANCH=\"${GIT_BRANCH}\"")
endif ()

addDefineToSource(source/api/fenestra_manager.cpp "FENESTRA_VERSION=\"${FENESTRA_APPLICATION_VERSION}\"")
addDefineToSource(source/api/fenestra_manager.cpp "FENESTRA_APPLICATION_NAME=\"${FENESTRA_APPLICATION_NAME}\"")

if (APPLE)
    enable_language(OBJC)
    target_sources(libfenestra PRIVATE source/helpers/utils_macos.m)
endif()

if (WIN32)
    set_target_properties(libfenestra PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    target_link_options(libfenestra PRIVATE -Wl,--export-all-symbols)
elseif (APPLE)
    find_library(FOUNDATION NAMES Foundation)
    target_link_libraries(libfenestra PUBLIC ${FOUNDATION})
elseif (UNIX)
    target_link_libraries(libfenestra PUBLIC xdgpp)
endif ()

target_compile_definitions(libfenestra PRIVATE FENESTRA_PROJECT_NAME="${PROJECT_NAME}")
target_include_directories(libfenestra PUBLIC include)
target_link_libraries(libfenestra PUBLIC
        wolv::io
        jthread
        nlohmann_json
        fmt::fmt
        imgui_imgui imgui_imnodes imgui_implot imgui_color_text_editor imgui_cimgui imgui_custom
        lunasvg
        trace
)

set_target_properties(libfenestra PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(libfenestra PROPERTIES PREFIX "")