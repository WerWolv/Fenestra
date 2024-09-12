project(main)

set(CMAKE_CXX_STANDARD 23)

add_executable(main
    source/main.cpp
    source/crash_handlers.cpp

    source/window/window.cpp
    source/window/impl/windows.cpp
    source/window/impl/macos.cpp
    source/window/impl/linux.cpp
    source/window/impl/web.cpp

    source/messaging/common.cpp
    source/messaging/linux.cpp
    source/messaging/macos.cpp
    source/messaging/windows.cpp
    source/messaging/web.cpp

    source/init/splash_window.cpp
    source/init/tasks.cpp

    source/init/run/common.cpp
    source/init/run/native.cpp
    source/init/run/web.cpp
    source/init/run/cli.cpp
)

target_include_directories(main PUBLIC include)

add_dependencies(fenestra_all main)

if (WIN32)
    set(FENESTRA_APPLICATION_NAME "${FENESTRA_APPLICATION_NAME_LOWER}-gui")
else ()
    set(FENESTRA_APPLICATION_NAME "${FENESTRA_APPLICATION_NAME_LOWER}")
endif ()

if (APPLE AND FENESTRA_MACOS_CREATE_BUNDLE)
    set_target_properties(main PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if (WIN32 AND NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set_target_properties(main PROPERTIES WIN32_EXECUTABLE ON)
endif()


if (EMSCRIPTEN)
    target_link_options(main PRIVATE -sGL_ENABLE_GET_PROC_ADDRESS)
    target_link_options(main PRIVATE -sUSE_PTHREADS=1 -sALLOW_MEMORY_GROWTH=1)
    target_link_options(main PRIVATE -sTOTAL_MEMORY=134217728)
    target_link_options(main PRIVATE -sMAX_WEBGL_VERSION=2)
    target_link_options(main PRIVATE -sEXPORTED_RUNTIME_METHODS=ccall)
    target_link_options(main PRIVATE -sFETCH)
    target_link_options(main PRIVATE -sWASM_BIGINT)
    target_link_options(main PRIVATE -O1)
    target_link_options(main PRIVATE -sLEGACY_GL_EMULATION)
    target_link_options(main PRIVATE -fsanitize=null)
    target_link_options(main PRIVATE -fsanitize-minimal-runtime)
    target_link_libraries(main PRIVATE idbfs.js)
endif ()

set_target_properties(main PROPERTIES
    OUTPUT_NAME "${FENESTRA_APPLICATION_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${FENESTRA_MAIN_OUTPUT_DIRECTORY}"
)

target_compile_definitions(main PRIVATE FENESTRA_PROJECT_NAME="${PROJECT_NAME}")

target_link_libraries(main PRIVATE libfenestra libwolv)
if (WIN32)
    target_link_libraries(main PRIVATE usp10 wsock32 ws2_32 Dwmapi.lib Winmm.lib DbgHelp.lib)
endif ()

if (APPLE)
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif ()