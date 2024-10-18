cmake_minimum_required(VERSION 3.16)

include(FenestraPlugin)

add_fenestra_plugin(
    SOURCES
        source/interface.cpp
        source/window_decoration.cpp
        source/themes.cpp
    INCLUDES
        include
    LIBRARIES
        fonts
)
