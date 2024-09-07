cmake_minimum_required(VERSION 3.16)
project(imgui)

set(CMAKE_CXX_STANDARD 17)

add_library(imgui_all_includes INTERFACE)
add_library(imgui_all INTERFACE)

add_subdirectory(imgui)
add_subdirectory(cimgui)
add_subdirectory(implot)
add_subdirectory(imnodes)
add_subdirectory(custom)
add_subdirectory(ColorTextEditor)

target_link_libraries(imgui_all INTERFACE imgui imgui_cimgui imgui_implot imgui_imnodes imgui_custom imgui_color_text_editor)
