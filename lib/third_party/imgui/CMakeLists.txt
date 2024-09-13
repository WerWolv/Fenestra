cmake_minimum_required(VERSION 3.16)
project(imgui)

set(CMAKE_CXX_STANDARD 17)

add_library(imgui_all INTERFACE)
add_library(imgui::all ALIAS imgui_all)

add_library(imgui_all_includes INTERFACE)
add_library(imgui::all_includes ALIAS imgui_all_includes)

add_subdirectory(imgui)
add_subdirectory(cimgui)
add_subdirectory(implot)
add_subdirectory(imnodes)
add_subdirectory(custom)
add_subdirectory(ColorTextEditor)

target_link_libraries(imgui_all INTERFACE imgui::imgui imgui::cimgui imgui::implot imgui::imnodes imgui::backend imgui::color_text_editor)
