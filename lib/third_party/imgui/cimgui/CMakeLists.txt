cmake_minimum_required(VERSION 3.16)

# https://github.com/cimgui/cimgui
project(imgui_cimgui)

set(CMAKE_CXX_STANDARD 17)

if (NOT FENESTRA_EXTERNAL_PLUGIN_BUILD)
    add_library(imgui_cimgui OBJECT
        source/cimgui.cpp
    )
    add_library(imgui::cimgui ALIAS imgui_cimgui)

    target_include_directories(imgui_cimgui PUBLIC
        include
    )

    target_link_libraries(imgui_cimgui PRIVATE imgui_includes)
    target_compile_definitions(imgui_cimgui PUBLIC IMGUI_LIBRARY_CIMGUI=1)
    add_dependencies(fenestra_all imgui_cimgui)
endif()

