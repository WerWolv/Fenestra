project(third_party)

set(MESSAGES_SUPPRESSED OFF)
set(BUILD_SHARED_LIBS OFF)

add_external_library(NAME imgui TARGET imgui_all NO_SYSTEM)
add_external_library(NAME jthread TARGET jthread)
add_external_library(NAME nlohmann_json TARGET nlohmann_json)
add_external_library(NAME xdgpp TARGET xdgpp)
add_external_library(NAME fmt TARGET fmt::fmt)
add_external_library(NAME lunasvg TARGET lunasvg)

set(SDL_WAYLAND_LIBDECOR ON CACHE INTERNAL "")
add_external_library(NAME SDL TARGET SDL3-static)

target_compile_definitions(lunasvg PUBLIC LUNASVG_BUILD_STATIC)
target_compile_options(SDL3-static PRIVATE "-Wno-deprecated-declarations")

set(MESSAGES_SUPPRESSED OFF)