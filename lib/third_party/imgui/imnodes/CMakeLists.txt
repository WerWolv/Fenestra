cmake_minimum_required(VERSION 3.16)
# https://github.com/Nelarius/imnodes
project(imgui_imnodes)

set(CMAKE_CXX_STANDARD 17)

if (NOT FENESTRA_EXTERNAL_PLUGIN_BUILD)
    add_library(imgui_imnodes OBJECT
        source/imnodes.cpp
    )
    add_library(imgui::imnodes ALIAS imgui_imnodes)

    target_include_directories(imgui_imnodes PUBLIC
        include
    )

    target_link_libraries(imgui_imnodes PRIVATE imgui_includes)
    target_compile_definitions(imgui_imnodes PUBLIC IMGUI_LIBRARY_IMNODES=1)
endif()

target_include_directories(imgui_all_includes INTERFACE include)
