cmake_minimum_required(VERSION 3.16)

include(FenestraPlugin)

add_fenestra_plugin(
    NAME
        bundled
    SOURCES
        source/interface.cpp
        source/window_decoration.cpp
    INCLUDES
        include
    LIBRARIES
        fonts
)
