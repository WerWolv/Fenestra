macro(add_fenestra_plugin)
    # Parse arguments
    set(options LIBRARY_PLUGIN)
    set(oneValueArgs FENESTRA_VERSION)
    set(multiValueArgs SOURCES INCLUDES LIBRARIES FEATURES)
    cmake_parse_arguments(FENESTRA_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_filename_component(FENESTRA_PLUGIN_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

    if (FENESTRA_PLUGIN_FENESTRA_VERSION)
        message(STATUS "Compiling plugin ${FENESTRA_PLUGIN_NAME} for Application Version ${FENESTRA_PLUGIN_FENESTRA_VERSION}")
        set(FENESTRA_VERSION_STRING "${FENESTRA_PLUGIN_FENESTRA_VERSION}")
    endif()

    set(FENESTRA_PLUGIN_NAME_RAW "${FENESTRA_PLUGIN_NAME}")
    set(FENESTRA_PLUGIN_NAME "${FENESTRA_PLUGIN_NAME}_plugin")

    if (FENESTRA_STATIC_LINK_PLUGINS)
        set(FENESTRA_PLUGIN_LIBRARY_TYPE STATIC)

        target_link_libraries(libfenestra PUBLIC ${FENESTRA_PLUGIN_NAME})

        configure_file(${FENESTRA_BASE_FOLDER}/dist/web/plugin_bundle.cpp.in ${FENESTRA_MAIN_OUTPUT_DIRECTORY}/plugin-bundle.cpp @ONLY)
        target_sources(main PUBLIC ${FENESTRA_MAIN_OUTPUT_DIRECTORY}/plugin-bundle.cpp)
        set(FENESTRA_PLUGIN_SUFFIX ".feneplugin")
    else()
        if (FENESTRA_PLUGIN_LIBRARY_PLUGIN)
            set(FENESTRA_PLUGIN_LIBRARY_TYPE SHARED)
            set(FENESTRA_PLUGIN_SUFFIX ".fenepluginlib")
        else()
            set(FENESTRA_PLUGIN_LIBRARY_TYPE MODULE)
            set(FENESTRA_PLUGIN_SUFFIX ".feneplugin")
        endif()
    endif()

    # Define new project for plugin
    project(${FENESTRA_PLUGIN_NAME})

    # Create a new shared library for the plugin source code
    add_library(${FENESTRA_PLUGIN_NAME} ${FENESTRA_PLUGIN_LIBRARY_TYPE} ${FENESTRA_PLUGIN_SOURCES})

    if (FENESTRA_PLUGIN_LIBRARY_PLUGIN)
        add_library(${FENESTRA_PLUGIN_NAME_RAW} ALIAS ${FENESTRA_PLUGIN_NAME})
    endif()

    # Add include directories and link libraries
    target_include_directories(${FENESTRA_PLUGIN_NAME} PUBLIC ${FENESTRA_PLUGIN_INCLUDES})
    target_link_libraries(${FENESTRA_PLUGIN_NAME} PUBLIC ${FENESTRA_PLUGIN_LIBRARIES})
    target_link_libraries(${FENESTRA_PLUGIN_NAME} PRIVATE libfenestra ${FMT_LIBRARIES} imgui_all_includes libwolv)

    precompileHeaders(${FENESTRA_PLUGIN_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/include")

    # Add FENESTRA_PROJECT_NAME and FENESTRA_VERSION define
    target_compile_definitions(${FENESTRA_PLUGIN_NAME} PRIVATE FENESTRA_PROJECT_NAME="${FENESTRA_PLUGIN_NAME_RAW}")
    target_compile_definitions(${FENESTRA_PLUGIN_NAME} PRIVATE FENESTRA_VERSION="${FENESTRA_VERSION_STRING}")
    target_compile_definitions(${FENESTRA_PLUGIN_NAME} PRIVATE FENESTRA_PLUGIN_NAME=${FENESTRA_PLUGIN_NAME_RAW})

    set_target_properties(${FENESTRA_PLUGIN_NAME} PROPERTIES OUTPUT_NAME ${FENESTRA_PLUGIN_NAME_RAW})

    # Enable required compiler flags
    enableUnityBuild(${FENESTRA_PLUGIN_NAME})
    setupCompilerFlags(${FENESTRA_PLUGIN_NAME})
    setupCompilerFlags(${FENESTRA_PLUGIN_NAME})

    # Configure build properties
    set_target_properties(${FENESTRA_PLUGIN_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${FENESTRA_MAIN_OUTPUT_DIRECTORY}/plugins"
            LIBRARY_OUTPUT_DIRECTORY "${FENESTRA_MAIN_OUTPUT_DIRECTORY}/plugins"
            CXX_STANDARD 23
            PREFIX ""
            SUFFIX ${FENESTRA_PLUGIN_SUFFIX}
    )

    # Set rpath of plugin libraries to the plugins folder
    if (APPLE)
        set_target_properties(${FENESTRA_PLUGIN_NAME} PROPERTIES BUILD_RPATH "@executable_path/../Frameworks;@executable_path/plugins")
    endif()

    # Setup a romfs for the plugin
    list(APPEND LIBROMFS_RESOURCE_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/romfs)
    set(LIBROMFS_PROJECT_NAME ${FENESTRA_PLUGIN_NAME_RAW})
    add_subdirectory(${FENESTRA_BASE_FOLDER}/lib/external/libromfs ${CMAKE_CURRENT_BINARY_DIR}/libromfs)
    target_link_libraries(${FENESTRA_PLUGIN_NAME} PRIVATE ${LIBROMFS_LIBRARY})

    set(FEATURE_DEFINE_CONTENT)

    if (FENESTRA_PLUGIN_FEATURES)
        list(LENGTH FENESTRA_PLUGIN_FEATURES FENESTRA_FEATURE_COUNT)
        math(EXPR FENESTRA_FEATURE_COUNT "${FENESTRA_FEATURE_COUNT} - 1" OUTPUT_FORMAT DECIMAL)
        foreach(index RANGE 0 ${FENESTRA_FEATURE_COUNT} 2)
            list(SUBLIST FENESTRA_PLUGIN_FEATURES ${index} 2 FENESTRA_PLUGIN_FEATURE)
            list(GET FENESTRA_PLUGIN_FEATURE 0 feature_define)
            list(GET FENESTRA_PLUGIN_FEATURE 1 feature_description)

            string(TOUPPER ${feature_define} feature_define)
            add_definitions(-DFENESTRA_PLUGIN_${FENESTRA_PLUGIN_NAME_RAW}_FEATURE_${feature_define}=0)
            set(FEATURE_DEFINE_CONTENT "${FEATURE_DEFINE_CONTENT}{ \"${feature_description}\", FENESTRA_FEATURE_ENABLED(${feature_define}) },")
        endforeach()
    endif()

    target_compile_options(${FENESTRA_PLUGIN_NAME} PRIVATE -DFENESTRA_PLUGIN_FEATURES_CONTENT=${FEATURE_DEFINE_CONTENT})

    # Add the new plugin to the main dependency list so it gets built by default
    if (TARGET fenestra_all)
        add_dependencies(fenestra_all ${FENESTRA_PLUGIN_NAME})
    endif()

    if (FENESTRA_EXTERNAL_PLUGIN_BUILD)
        install(
            TARGETS
                ${FENESTRA_PLUGIN_NAME}
            RUNTIME DESTINATION "."
            LIBRARY DESTINATION "."
        )
    endif()

    # Fix rpath
    if (APPLE)
        set_target_properties(
                ${FENESTRA_PLUGIN_NAME}
                PROPERTIES
                    INSTALL_RPATH "@executable_path/../Frameworks;@executable_path/plugins"
        )
    elseif (UNIX)
        set(PLUGIN_RPATH "")
        list(APPEND PLUGIN_RPATH "$ORIGIN")

        if (FENESTRA_PLUGIN_ADD_INSTALL_PREFIX_TO_RPATH)
            list(APPEND PLUGIN_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
        endif()

        set_target_properties(
                ${FENESTRA_PLUGIN_NAME}
                PROPERTIES
                    INSTALL_RPATH_USE_ORIGIN ON
                    INSTALL_RPATH "${PLUGIN_RPATH}"
        )
    endif()

    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt AND FENESTRA_ENABLE_UNIT_TESTS AND FENESTRA_ENABLE_PLUGIN_TESTS)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
        target_link_libraries(${FENESTRA_PLUGIN_NAME} PUBLIC ${FENESTRA_PLUGIN_NAME}_tests)
        target_compile_definitions(${FENESTRA_PLUGIN_NAME}_tests PRIVATE FENESTRA_PROJECT_NAME="${FENESTRA_PLUGIN_NAME_RAW}-tests")
    endif()
endmacro()

macro(add_romfs_resource input output)
    if (NOT EXISTS ${input})
        message(WARNING "Resource file ${input} does not exist")
    endif()

    configure_file(${input} ${CMAKE_CURRENT_BINARY_DIR}/romfs/${output} COPYONLY)

    list(APPEND LIBROMFS_RESOURCE_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/romfs)
endmacro()

macro (enable_plugin_feature feature)
    string(TOUPPER ${feature} feature)
    if (NOT (feature IN_LIST FENESTRA_PLUGIN_FEATURES))
        message(FATAL_ERROR "Feature ${feature} is not enabled for plugin ${FENESTRA_PLUGIN_NAME_RAW}")
    endif()

    remove_definitions(-DFENESTRA_PLUGIN_${FENESTRA_PLUGIN_NAME_RAW}_FEATURE_${feature}=0)
    add_definitions(-DFENESTRA_PLUGIN_${FENESTRA_PLUGIN_NAME_RAW}_FEATURE_${feature}=1)
endmacro()