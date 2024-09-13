cmake_minimum_required(VERSION 3.16)
# https://github.com/BalazsJako/ImGuiColorTextEdit
project(imgui_color_text_editor)

set(CMAKE_CXX_STANDARD 17)

if (NOT FENESTRA_EXTERNAL_PLUGIN_BUILD)
    add_library(imgui_color_text_editor OBJECT
        source/TextEditor.cpp
    )
    add_library(imgui::color_text_editor ALIAS imgui_color_text_editor)

    target_include_directories(imgui_color_text_editor PUBLIC
        include
    )

    target_link_libraries(imgui_color_text_editor PRIVATE imgui_includes)
    target_compile_definitions(imgui_color_text_editor PUBLIC IMGUI_LIBRARY_COLOR_TEXT_EDITOR=1)
endif()

target_include_directories(imgui_all_includes INTERFACE include)

