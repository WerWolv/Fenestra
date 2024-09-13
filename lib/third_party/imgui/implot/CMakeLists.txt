cmake_minimum_required(VERSION 3.16)
# https://github.com/epezent/implot
project(imgui_implot)

set(CMAKE_CXX_STANDARD 17)

if (NOT FENESTRA_EXTERNAL_PLUGIN_BUILD)
    add_library(imgui_implot OBJECT
        source/implot.cpp
        source/implot_items.cpp
        source/implot_demo.cpp
    )
    add_library(imgui::implot ALIAS imgui_implot)

    target_include_directories(imgui_implot PUBLIC
        include
    )

    target_link_libraries(imgui_implot PRIVATE imgui_includes)
    target_compile_definitions(imgui_implot PUBLIC IMGUI_LIBRARY_IMPLOT=1)
endif()

target_include_directories(imgui_all_includes INTERFACE include)
