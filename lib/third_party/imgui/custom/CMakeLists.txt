cmake_minimum_required(VERSION 3.16)
# https://github.com/ocornut/imgui with custom modifications made to the OpenGL 3 and SDL3 backends
project(imgui_backend)

set(CMAKE_CXX_STANDARD 17)

if (NOT FENESTRA_EXTERNAL_PLUGIN_BUILD)
    add_library(imgui_backend OBJECT
        source/imgui_impl_opengl3.cpp
        source/imgui_impl_sdl3.cpp
    )
    add_library(imgui::backend ALIAS imgui_backend)

    target_include_directories(imgui_backend PUBLIC
        include
    )

    target_link_libraries(imgui_backend PRIVATE imgui_includes)

    find_package(OpenGL REQUIRED)

    target_include_directories(imgui_backend PUBLIC ${OpenGL_INCLUDE_DIRS})
    target_link_directories(imgui_backend PUBLIC ${OpenGL_LIBRARY_DIRS})
    target_link_libraries(imgui_backend PUBLIC SDL3::SDL3 ${OPENGL_LIBRARIES})
endif()

target_include_directories(imgui_all_includes INTERFACE include)