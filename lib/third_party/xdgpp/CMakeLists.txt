cmake_minimum_required(VERSION 3.16)

project(xdgpp)

add_library(xdgpp INTERFACE)
target_include_directories(jthread INTERFACE xdgpp)