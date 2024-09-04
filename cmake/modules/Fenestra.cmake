macro(add_fenestra_app)
    # Parse arguments
    set(options STRICT_WARNINGS)
    set(oneValueArgs NAME AUTHOR PLUGIN_DIRECTORY OUTPUT_DIRECTORY FENESTRA_PATH)
    set(multiValueArgs)
    cmake_parse_arguments(FENESTRA_APP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT FENESTRA_APP_NAME)
        message(FATAL_ERROR "APP_NAME name is required. Please specify the name of the application.")
    endif()
    set(FENESTRA_APPLICATION_NAME ${FENESTRA_APP_NAME})

    if (NOT FENESTRA_APP_AUTHOR)
        message(FATAL_ERROR "APP_AUTHOR is required. Please specify the author of the application.")
    endif()
    set(FENESTRA_APPLICATION_AUTHOR ${FENESTRA_APP_AUTHOR})

    if (NOT FENESTRA_APP_PLUGIN_DIRECTORY)
        message(FATAL_ERROR "PLUGIN_DIRECTORY is required. Please specify the folder where your plugins are located in.")
    endif()
    set(FENESTRA_APPLICATION_PLUGIN_DIRECTORY ${FENESTRA_APP_PLUGIN_DIRECTORY})

    if (STRICT_WARNINGS)
        set(FENESTRA_STRICT_WARNINGS ON)
    else()
        set(FENESTRA_STRICT_WARNINGS OFF)
    endif()

    if (FENESTRA_APP_OUTPUT_DIRECTORY)
        set(FENESTRA_MAIN_OUTPUT_DIRECTORY ${FENESTRA_MAIN_OUTPUT_DIRECTORY})
    else()
        set(FENESTRA_MAIN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${FENESTRA_APPLICATION_NAME}")
    endif()

    if (FENESTRA_APP_FENESTRA_PATH)
        add_subdirectory(${FENESTRA_APP_FENESTRA_PATH} ${FENESTRA_MAIN_OUTPUT_DIRECTORY})
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FENESTRA_APP_FENESTRA_PATH}/cmake/modules")
    else()
        include(FetchContent)
        FetchContent_Declare(
                fenestra
                GIT_REPOSITORY https://github.com/WerWolv/Fenestra.git
                GIT_TAG        main
        )
        FetchContent_MakeAvailable(fenestra)

        FetchContent_GetProperties(fenestra SOURCE_DIR FENESTRA_CLONE_PATH)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${FENESTRA_CLONE_PATH}/cmake/modules")
    endif()
endmacro()