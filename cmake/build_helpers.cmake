include(FetchContent)

function(message)
    if (MESSAGES_SUPPRESSED AND "${ARGV0}" STREQUAL "STATUS")
        return()
    endif()

    _message(${ARGV})
endfunction()

string(TOLOWER ${FENESTRA_APPLICATION_NAME} FENESTRA_APPLICATION_NAME_LOWER)

if (PROJECT_IS_TOP_LEVEL)
    set(FENESTRA_APPLICATION_NAME_LOWER ${FENESTRA_APPLICATION_NAME_LOWER} PARENT_SCOPE)
endif()

if(FENESTRA_STRIP_RELEASE)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CPACK_STRIP_FILES TRUE)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_link_options($<$<CONFIG:RELEASE>:-s>)
    endif()
endif()

macro(addDefines)
    if (NOT FENESTRA_APPLICATION_VERSION)
        message(FATAL_ERROR "FENESTRA_APPLICATION_VERSION is not defined")
    endif ()

    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR} -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH} ")

    set(FENESTRA_APPLICATION_VERSION_STRING ${FENESTRA_APPLICATION_VERSION})
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(FENESTRA_APPLICATION_VERSION_STRING ${FENESTRA_APPLICATION_VERSION_STRING})
        add_compile_definitions(NDEBUG)
    elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(FENESTRA_APPLICATION_VERSION_STRING ${FENESTRA_APPLICATION_VERSION_STRING}-Debug)
        add_compile_definitions(DEBUG)
    elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(FENESTRA_APPLICATION_VERSION_STRING ${FENESTRA_APPLICATION_VERSION_STRING})
        add_compile_definitions(NDEBUG)
    elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(FENESTRA_APPLICATION_VERSION_STRING ${FENESTRA_APPLICATION_VERSION_STRING}-MinSizeRel)
        add_compile_definitions(NDEBUG)
    endif ()

    if (FENESTRA_ENABLE_STD_ASSERTS)
        add_compile_definitions(_GLIBCXX_DEBUG _GLIBCXX_VERBOSE)
    endif()

    if (FENESTRA_STATIC_LINK_PLUGINS)
        add_compile_definitions(FENESTRA_STATIC_LINK_PLUGINS)
    endif ()

    add_compile_definitions(FENESTRA_APPLICATION_NAME="${FENESTRA_APPLICATION_NAME}")
    add_compile_definitions(FENESTRA_APPLICATION_NAME_LOWER="${FENESTRA_APPLICATION_NAME_LOWER}")
endmacro()

function(addDefineToSource SOURCE DEFINE)
    set_property(
            SOURCE ${SOURCE}
            APPEND
            PROPERTY COMPILE_DEFINITIONS "${DEFINE}"
    )

    # Disable precompiled headers for this file
    set_source_files_properties(${SOURCE} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endfunction()

# Detect current OS / System
macro(detectOS)
    if (WIN32)
        add_compile_definitions(OS_WINDOWS)
        set(CMAKE_INSTALL_BINDIR ".")
        set(CMAKE_INSTALL_LIBDIR ".")
        set(PLUGINS_INSTALL_LOCATION "plugins")
        add_compile_definitions(WIN32_LEAN_AND_MEAN)
        add_compile_definitions(UNICODE)
    elseif (APPLE)
        add_compile_definitions(OS_MACOS)
        set(CMAKE_INSTALL_BINDIR ".")
        set(CMAKE_INSTALL_LIBDIR ".")
        set(PLUGINS_INSTALL_LOCATION "plugins")
        enable_language(OBJC)
        enable_language(OBJCXX)
    elseif (EMSCRIPTEN)
        add_compile_definitions(OS_WEB)
    elseif (UNIX AND NOT APPLE)
        add_compile_definitions(OS_LINUX)
        if (BSD AND BSD STREQUAL "FreeBSD")
            add_compile_definitions(OS_FREEBSD)
        endif()
        include(GNUInstallDirs)

        if(FENESTRA_APPLICATION_PLUGINS_IN_SHARE)
            set(PLUGINS_INSTALL_LOCATION "share/${FENESTRA_APPLICATION_NAME_LOWER}/plugins")
        else()
            set(PLUGINS_INSTALL_LOCATION "${CMAKE_INSTALL_LIBDIR}/${FENESTRA_APPLICATION_NAME_LOWER}/plugins")

            # Add System plugin location for plugins to be loaded from
            # IMPORTANT: This does not work for Sandboxed or portable builds such as the Flatpak or AppImage release
            add_compile_definitions(SYSTEM_PLUGINS_LOCATION="${CMAKE_INSTALL_FULL_LIBDIR}/${FENESTRA_APPLICATION_NAME_LOWER}")
        endif()

    else ()
        message(FATAL_ERROR "Unknown / unsupported system!")
    endif()

endmacro()

macro(addPluginDirectories)
    file(MAKE_DIRECTORY "${FENESTRA_PLUGIN_FOLDER}")

    foreach (plugin IN ZIP_LISTS PLUGINS PLUGINS_PATHS)
        set(plugin "${plugin_0}_plugin")
        set(plugin_path "${plugin_1}")

        add_subdirectory("${plugin_path}" "${CMAKE_CURRENT_BINARY_DIR}/plugins/${plugin}")

        if (TARGET ${plugin})
            set_target_properties(${plugin} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FENESTRA_MAIN_OUTPUT_DIRECTORY}/plugins")
            set_target_properties(${plugin} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${FENESTRA_MAIN_OUTPUT_DIRECTORY}/plugins")

            if (APPLE)
                if (FENESTRA_GENERATE_PACKAGE)
                    set_target_properties(${plugin} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGINS_INSTALL_LOCATION})
                endif ()
            else ()
                if (WIN32)
                    get_target_property(target_type ${plugin} TYPE)
                    if (${target_type} STREQUAL "MODULE_LIBRARY")
                        install(TARGETS ${plugin} LIBRARY DESTINATION ${PLUGINS_INSTALL_LOCATION})
                    else ()
                        install(TARGETS ${plugin} RUNTIME DESTINATION ${PLUGINS_INSTALL_LOCATION})
                    endif()
                else()
                    install(TARGETS ${plugin} LIBRARY DESTINATION ${PLUGINS_INSTALL_LOCATION})
                endif()

            endif()

            add_dependencies(fenestra_all ${plugin})
        endif ()
    endforeach()
endmacro()

macro(configureCMake)
    message(STATUS "Configuring ${FENESTRA_APPLICATION_NAME} v${FENESTRA_APPLICATION_VERSION}")

    # Enable C and C++ languages
    enable_language(C CXX)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable position independent code for all targets" FORCE)

    # Configure use of recommended build tools
    if (FENESTRA_USE_DEFAULT_BUILD_SETTINGS)
        message(STATUS "Configuring CMake to use recommended build tools...")

        find_program(CCACHE_PATH ccache)
        find_program(NINJA_PATH ninja)
        find_program(LD_LLD_PATH ld.lld)
        find_program(AR_LLVMLIBS_PATH llvm-ar)
        find_program(RANLIB_LLVMLIBS_PATH llvm-ranlib)

        if (CCACHE_PATH)
            set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PATH})
            set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PATH})
        else ()
            message(WARNING "ccache not found!")
        endif ()

        if (AR_LLVMLIBS_PATH)
            set(CMAKE_AR ${AR_LLVMLIBS_PATH})
        else ()
            message(WARNING "llvm-ar not found, using default ar!")
        endif ()

        if (RANLIB_LLVMLIBS_PATH)
            set(CMAKE_RANLIB ${RANLIB_LLVMLIBS_PATH})
        else ()
            message(WARNING "llvm-ranlib not found, using default ranlib!")
        endif ()

        if (LD_LLD_PATH)
            set(CMAKE_LINKER ${LD_LLD_PATH})

            if (NOT XCODE)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=lld")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=lld")
            endif()
        else ()
            message(WARNING "lld not found, using default linker!")
        endif ()

        if (NINJA_PATH)
            set(CMAKE_GENERATOR Ninja)
        else ()
            message(WARNING "ninja not found, using default generator!")
        endif ()
    endif()

    # Enable LTO if desired and supported
    if (FENESTRA_ENABLE_LTO)
        include(CheckIPOSupported)

        check_ipo_supported(RESULT result OUTPUT output_error)
        if (result)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
            message(STATUS "LTO enabled!")
        else ()
            message(WARNING "LTO is not supported: ${output_error}")
        endif ()
    endif ()

    # Some libraries we use set the BUILD_SHARED_LIBS variable to ON, which causes CMake to
    # display a warning about options being set using set() instead of option().
    # Explicitly set the policy to NEW to suppress the warning.
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Disable deprecated warnings" FORCE)
endmacro()

function(configureProject)
    if (XCODE)
        # Support Xcode's multi configuration paradigm by placing built artifacts into separate directories
        set(FENESTRA_MAIN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Configs/$<CONFIG>" PARENT_SCOPE)
    else()
        set(FENESTRA_MAIN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
    endif()
endfunction()

macro(setDefaultBuiltTypeIfUnset)
    if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Using RelWithDebInfo build type as it was left unset" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "RelWithDebInfo")
    endif()
endmacro()

function(loadVersion version plain_version)
    set(VERSION_FILE "${FENESTRA_APPLICATION_BASE_FOLDER}/VERSION")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${VERSION_FILE})
    file(READ "${VERSION_FILE}" read_version)
    string(STRIP ${read_version} read_version)
    string(REPLACE ".WIP" "" read_version_plain ${read_version})
    set(${version} ${read_version} PARENT_SCOPE)
    set(${plain_version} ${read_version_plain} PARENT_SCOPE)
endfunction()

function(detectBadClone)
    if (FENESTRA_IGNORE_BAD_CLONE)
        return()
    endif()

    file (GLOB EXTERNAL_DIRS "lib/external/*" "lib/third_party/*")
    foreach (EXTERNAL_DIR ${EXTERNAL_DIRS})
        file(GLOB_RECURSE RESULT "${EXTERNAL_DIR}/*")
        list(LENGTH RESULT ENTRY_COUNT)
        if(ENTRY_COUNT LESS_EQUAL 1)
            message(FATAL_ERROR "External dependency ${EXTERNAL_DIR} is empty!\nMake sure to correctly clone the git repo using the --recurse-submodules git option or initialize the submodules manually.")
        endif()
    endforeach ()
endfunction()

macro(detectBundledPlugins)
    if (NOT FENESTRA_PLUGIN_FOLDER)
        set(FENESTRA_PLUGIN_FOLDER "plugins")
    endif()

    if (NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR}))
        if (NOT FENESTRA_APPLICATION_PLUGIN_DIRECTORY)
            message(FATAL_ERROR "FENESTRA_APPLICATION_PLUGIN_DIRECTORY is not defined! Please set it to your application's own plugin directory.")
        endif()

        if (NOT EXISTS "${FENESTRA_APPLICATION_PLUGIN_DIRECTORY}")
            message(FATAL_ERROR "Application plugins directory '${FENESTRA_APPLICATION_PLUGIN_DIRECTORY}' not found! Please set it to an existing directory path")
        endif()

        file(GLOB APPLICATION_PLUGIN_DIRS "${FENESTRA_APPLICATION_PLUGIN_DIRECTORY}/*")
    endif()

    file(GLOB BUILTIN_PLUGINS_DIRS "${FENESTRA_PLUGIN_FOLDER}/*")

    set(PLUGINS_DIRS "${BUILTIN_PLUGINS_DIRS};${APPLICATION_PLUGIN_DIRS}")

    if (NOT DEFINED FENESTRA_INCLUDE_PLUGINS)
        foreach(PLUGIN_DIR ${PLUGINS_DIRS})
            if (EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
                get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
                if (NOT (${PLUGIN_NAME} IN_LIST FENESTRA_EXCLUDE_PLUGINS))
                    list(APPEND PLUGINS ${PLUGIN_NAME})
                    list(APPEND PLUGINS_PATHS ${PLUGIN_DIR})
                endif ()
            endif()
        endforeach()
    else()
        foreach(PLUGIN_DIR ${PLUGINS_DIRS})
            if (EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
                get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
                if (${PLUGIN_NAME} IN_LIST FENESTRA_INCLUDE_PLUGINS)
                    list(APPEND PLUGINS ${PLUGIN_NAME})
                    list(APPEND PLUGINS_PATHS ${PLUGIN_DIR})
                endif ()
            endif()
        endforeach()
    endif()

    foreach(PLUGIN_NAME ${PLUGINS})
        message(STATUS "Enabled bundled plugin '${PLUGIN_NAME}'")
    endforeach()
endmacro()

macro(setVariableInParent variable value)
    get_directory_property(hasParent PARENT_DIRECTORY)

    if (hasParent)
        set(${variable} "${value}" PARENT_SCOPE)
    else ()
        set(${variable} "${value}")
    endif ()
endmacro()

# Compress debug info. See https://github.com/WerWolv/ImHex/issues/1714 for relevant problem
macro(setupDebugCompressionFlag)
    include(CheckCXXCompilerFlag)
    include(CheckLinkerFlag)

    check_cxx_compiler_flag(-gz=zstd ZSTD_AVAILABLE_COMPILER)
    check_linker_flag(CXX -gz=zstd ZSTD_AVAILABLE_LINKER)
    check_cxx_compiler_flag(-gz COMPRESS_AVAILABLE_COMPILER)
    check_linker_flag(CXX -gz COMPRESS_AVAILABLE_LINKER)

    if (NOT DEBUG_COMPRESSION_FLAG) # Cache variable
        if (ZSTD_AVAILABLE_COMPILER AND ZSTD_AVAILABLE_LINKER)
            message("Using Zstd compression for debug info because both compiler and linker support it")
            set(DEBUG_COMPRESSION_FLAG "-gz=zstd" CACHE STRING "Cache to use for debug info compression")
        elseif (COMPRESS_AVAILABLE_COMPILER AND COMPRESS_AVAILABLE_LINKER)
            message("Using default compression for debug info because both compiler and linker support it")
            set(DEBUG_COMPRESSION_FLAG "-gz" CACHE STRING "Cache to use for debug info compression")
        endif()
    endif()

    set(FENESTRA_COMMON_FLAGS "${FENESTRA_COMMON_FLAGS} ${DEBUG_COMPRESSION_FLAG}")
endmacro()

macro(setupCompilerFlags target)
    # FENESTRA_COMMON_FLAGS: flags common for C, C++, Objective C, etc.. compilers

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Define strict compilation flags
        if (FENESTRA_STRICT_WARNINGS)
            set(FENESTRA_COMMON_FLAGS "${FENESTRA_COMMON_FLAGS} -Wall -Wextra -Wpedantic -Werror")
        endif()

        if (UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            set(FENESTRA_COMMON_FLAGS "${FENESTRA_COMMON_FLAGS} -rdynamic")
        endif()

        set(FENESTRA_CXX_FLAGS "-fexceptions -frtti")

        # Disable some warnings
        set(FENESTRA_C_CXX_FLAGS "-Wno-unknown-warning-option -Wno-array-bounds -Wno-deprecated-declarations -Wno-unknown-pragmas")
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if (FENESTRA_ENABLE_UNITY_BUILD AND WIN32)
            set(FENESTRA_COMMON_FLAGS "${FENESTRA_COMMON_FLAGS} -Wa,-mbig-obj")
        endif ()

        # Disable some warnings for gcc
        set(FENESTRA_C_CXX_FLAGS "${FENESTRA_C_CXX_FLAGS} -Wno-restrict -Wno-stringop-overread -Wno-stringop-overflow -Wno-dangling-reference")
    endif()

    # Define emscripten-specific disabled warnings
    if (EMSCRIPTEN)
        set(FENESTRA_C_CXX_FLAGS "${FENESTRA_C_CXX_FLAGS} -pthread -Wno-dollar-in-identifier-extension -Wno-pthreads-mem-growth")
    endif ()

    if (FENESTRA_COMPRESS_DEBUG_INFO)
        setupDebugCompressionFlag()
    endif()

    # Set actual CMake flags
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS "${FENESTRA_COMMON_FLAGS} ${FENESTRA_C_CXX_FLAGS}")
    set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS}    ${FENESTRA_COMMON_FLAGS} ${FENESTRA_C_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  ${FENESTRA_COMMON_FLAGS} ${FENESTRA_C_CXX_FLAGS}  ${FENESTRA_CXX_FLAGS}")
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} ${FENESTRA_COMMON_FLAGS}")

    # Only generate minimal debug information for stacktraces in RelWithDebInfo builds
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g1")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Add flags for debug info in inline functions
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -gstatement-frontiers -ginline-points")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -gstatement-frontiers -ginline-points")
    endif()
endmacro()

# uninstall target
macro(setUninstallTarget)
    if(NOT TARGET uninstall)
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                IMMEDIATE @ONLY)

        add_custom_target(uninstall
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endmacro()

macro(addBundledLibraries)
    set(FPHSA_NAME_MISMATCHED ON CACHE BOOL "")

    if (NOT FENESTRA_DISABLE_STACKTRACE)
        if (WIN32)
            message(STATUS "StackWalk enabled!")
            set(LIBBACKTRACE_LIBRARIES DbgHelp.lib)
        else ()
            find_package(Backtrace)
            if (${Backtrace_FOUND})
                message(STATUS "Backtrace enabled! Header: ${Backtrace_HEADER}")

                if (Backtrace_HEADER STREQUAL "backtrace.h")
                    set(LIBBACKTRACE_LIBRARIES ${Backtrace_LIBRARY})
                    set(LIBBACKTRACE_INCLUDE_DIRS ${Backtrace_INCLUDE_DIR})
                    add_compile_definitions(BACKTRACE_HEADER=<${Backtrace_HEADER}>)
                    add_compile_definitions(FENESTRA_HAS_BACKTRACE)
                elseif (Backtrace_HEADER STREQUAL "execinfo.h")
                    set(LIBBACKTRACE_LIBRARIES ${Backtrace_LIBRARY})
                    set(LIBBACKTRACE_INCLUDE_DIRS ${Backtrace_INCLUDE_DIR})
                    add_compile_definitions(BACKTRACE_HEADER=<${Backtrace_HEADER}>)
                    add_compile_definitions(FENESTRA_HAS_EXECINFO)
                endif()
            endif()
        endif()
    endif()

    foreach (library IN LISTS FENESTRA_BUNDLED_IMGUI_LIBRARIES)
        message(STATUS "Enabling ImGui library '${library}'")
    endforeach()
endmacro()

function(enableUnityBuild TARGET)
    if (FENESTRA_ENABLE_UNITY_BUILD)
        set_target_properties(${TARGET} PROPERTIES UNITY_BUILD ON UNITY_BUILD_MODE BATCH)
    endif ()
endfunction()

function(generatePDBs)
    if (NOT FENESTRA_GENERATE_PDBS)
        return()
    endif ()

    if (NOT WIN32 OR CMAKE_BUILD_TYPE STREQUAL "Debug")
        return()
    endif ()

    include(FetchContent)
    FetchContent_Declare(
            cv2pdb
            URL "https://github.com/rainers/cv2pdb/releases/download/v0.52/cv2pdb-0.52.zip"
            DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    FetchContent_Populate(cv2pdb)

    set(PDBS_TO_GENERATE main main-forwarder libfenestra ${PLUGINS})
    foreach (PDB ${PDBS_TO_GENERATE})
        if (PDB STREQUAL "main")
            set(GENERATED_PDB ${FENESTRA_APPLICATION_NAME_LOWER})
        elseif (PDB STREQUAL "main-forwarder")
            set(GENERATED_PDB ${FENESTRA_APPLICATION_NAME_LOWER}-gui)
        elseif (PDB STREQUAL "libfenestra")
            set(GENERATED_PDB libfenestra)
        else ()
            set(GENERATED_PDB plugins/${PDB})
        endif ()

        if (FENESTRA_REPLACE_DWARF_WITH_PDB)
            set(PDB_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${GENERATED_PDB})
        else ()
            set(PDB_OUTPUT_PATH)
        endif()

        add_custom_target(${PDB}_pdb DEPENDS ${CMAKE_BINARY_DIR}/${GENERATED_PDB}.pdb)
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${GENERATED_PDB}.pdb
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMAND
                (
                    ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${GENERATED_PDB}.pdb &&
                    ${cv2pdb_SOURCE_DIR}/cv2pdb64.exe $<TARGET_FILE:${PDB}> ${PDB_OUTPUT_PATH} &&
                    ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/${GENERATED_PDB}
                ) || (exit 0)
                COMMAND_EXPAND_LISTS)

        install(FILES ${CMAKE_BINARY_DIR}/${GENERATED_PDB}.pdb DESTINATION ".")

        add_dependencies(fenestra_all ${PDB}_pdb)
    endforeach ()

endfunction()

function(addIncludesFromLibrary target library)
    get_target_property(library_include_dirs ${library} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} PRIVATE ${library_include_dirs})
endfunction()

function(precompileHeaders target includeFolder)
    if (NOT FENESTRA_ENABLE_PRECOMPILED_HEADERS)
        return()
    endif()

    file(GLOB_RECURSE TARGET_INCLUDES "${includeFolder}/**/*.hpp")
    set(SYSTEM_INCLUDES "<algorithm>;<array>;<atomic>;<chrono>;<cmath>;<cstddef>;<cstdint>;<cstdio>;<cstdlib>;<cstring>;<exception>;<filesystem>;<functional>;<iterator>;<limits>;<list>;<map>;<memory>;<optional>;<ranges>;<set>;<stdexcept>;<string>;<string_view>;<thread>;<tuple>;<type_traits>;<unordered_map>;<unordered_set>;<utility>;<variant>;<vector>")
    set(INCLUDES "${SYSTEM_INCLUDES};${TARGET_INCLUDES}")
    string(REPLACE ">" "$<ANGLE-R>" INCLUDES "${INCLUDES}")
    target_precompile_headers(${target}
            PUBLIC
            "$<$<COMPILE_LANGUAGE:CXX>:${INCLUDES}>"
    )
endfunction()

function(add_external_library)
    set(options NO_SYSTEM)
    set(oneValueArgs NAME TARGET)
    set(multiValueArgs)
    cmake_parse_arguments(THIRD_PARTY_LIBRARY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    string(TOUPPER ${THIRD_PARTY_LIBRARY_NAME} THIRD_PARTY_LIBRARY_NAME_UPPER)

    if (FENESTRA_USE_SYSTEM_${THIRD_PARTY_LIBRARY_NAME_UPPER})
        if (THIRD_PARTY_LIBRARY_NO_SYSTEM)
            message(FATAL_ERROR "Requested use of system library '${THIRD_PARTY_LIBRARY_NAME}' but this is not possible with this library.\nThis is most likely because the library is custom, tightly integrated with Fenestra or modified.\nIf you're a Linux package maintainer, please don't try to circumvent this, it was done on purpose.")
        endif()

        if (DEFINED ${THIRD_PARTY_LIBRARY_NAME_UPPER}_PACKAGE_NAME)
            find_package(${${THIRD_PARTY_LIBRARY_NAME_UPPER}_PACKAGE_NAME} REQUIRED)
        else()
            if (NOT DEFINED FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_INCLUDE_DIRS)
                message(FATAL_ERROR "Use of system library '${THIRD_PARTY_LIBRARY_NAME}' was requested but FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_INCLUDE_DIRS was not defined")
            endif()

            if (NOT DEFINED FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_LIBRARY_DIRS)
                message(FATAL_ERROR "Use of system library '${THIRD_PARTY_LIBRARY_NAME}' was requested but FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_LIBRARY_DIRS was not defined")
            endif()

            if (NOT DEFINED FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_LIBRARIES)
                message(FATAL_ERROR "Use of system library '${THIRD_PARTY_LIBRARY_NAME}' was requested but FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_LIBRARIES was not defined")
            endif()
        endif()

        add_library(external_library_${THIRD_PARTY_LIBRARY_NAME} INTERFACE)
        add_library(${THIRD_PARTY_LIBRARY_TARGET} ALIAS external_library_${THIRD_PARTY_LIBRARY_NAME})
        target_include_directories(external_library_${THIRD_PARTY_LIBRARY_NAME} INTERFACE ${FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_INCLUDE_DIRS})
        target_link_directories(external_library_${THIRD_PARTY_LIBRARY_NAME} INTERFACE ${FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_LIBRARY_DIRS})
        target_link_libraries(external_library_${THIRD_PARTY_LIBRARY_NAME} INTERFACE ${FENESTRA_${THIRD_PARTY_LIBRARY_NAME_UPPER}_LIBRARIES})
    else()
        add_subdirectory(${THIRD_PARTY_LIBRARY_NAME} EXCLUDE_FROM_ALL)

        if (NOT TARGET ${THIRD_PARTY_LIBRARY_TARGET})
            message(FATAL_ERROR "Could not find target ${THIRD_PARTY_LIBRARY_TARGET}")
        endif()
    endif()
endfunction()

function(createPackage)
    if (NOT FENESTRA_DEFAULT_INSTALL_TARGETS)
        return()
    endif()

    if (WIN32)
        # Install binaries directly in the prefix, usually C:\Program Files\<Application>.
        set(CMAKE_INSTALL_BINDIR ".")

        set(PLUGIN_TARGET_FILES "")
        foreach (plugin IN LISTS PLUGINS)
            list(APPEND PLUGIN_TARGET_FILES "$<TARGET_FILE:${plugin}_plugin>")
        endforeach ()

        # Grab all dynamically linked dependencies.
        install(CODE "set(CMAKE_INSTALL_BINDIR \"${CMAKE_INSTALL_BINDIR}\")")
        install(CODE "set(PLUGIN_TARGET_FILES \"${PLUGIN_TARGET_FILES}\")")
        install(CODE [[
            file(GET_RUNTIME_DEPENDENCIES
                EXECUTABLES ${PLUGIN_TARGET_FILES} $<TARGET_FILE:libfenestra> $<TARGET_FILE:main>
                RESOLVED_DEPENDENCIES_VAR _r_deps
                UNRESOLVED_DEPENDENCIES_VAR _u_deps
                CONFLICTING_DEPENDENCIES_PREFIX _c_deps
                DIRECTORIES ${CMAKE_BINARY_DIR} ${DEP_FOLDERS} $ENV{PATH}
                POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
            )

            if(_c_deps_FILENAMES)
                message(WARNING "Conflicting dependencies for library: \"${_c_deps}\"!")
            endif()

            foreach(_file ${_r_deps})
                file(INSTALL
                    DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
                    TYPE SHARED_LIBRARY
                    FOLLOW_SYMLINK_CHAIN
                    FILES "${_file}"
                )
            endforeach()
        ]])
    elseif(UNIX AND NOT APPLE)
        set_target_properties(libfenestra PROPERTIES SOVERSION ${FENESTRA_APPLICATION_VERSION})
    endif()

    if (APPLE)
        set(BUNDLE_PATH "${CMAKE_INSTALL_PREFIX}/${FENESTRA_APPLICATION_NAME}.app")

        set_target_properties(libfenestra PROPERTIES SOVERSION ${FENESTRA_APPLICATION_VERSION})

        if (FENESTRA_MACOS_CREATE_BUNDLE)
            set_property(TARGET main PROPERTY MACOSX_BUNDLE_INFO_PLIST ${MACOSX_BUNDLE_INFO_PLIST})

            install(TARGETS main BUNDLE DESTINATION ".")

            install(TARGETS ${PLUGINS} LIBRARY DESTINATION "$<TARGET_FILE_DIR:main>/plugins")
            install(CODE "set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")")
            install(CODE "set(FENESTRA_APPLICATION_NAME \"${FENESTRA_APPLICATION_NAME}\")")
            install(CODE "set(BUNDLE_PATH \"${BUNDLE_PATH}\")")
            install(CODE [[
                include(BundleUtilities)

                set(PLUGINS_PATH "${BUNDLE_PATH}/Contents/MacOS/plugins")
                copy_and_fixup_bundle("${CMAKE_CURRENT_BINARY_DIR}/${FENESTRA_APPLICATION_NAME}.app" "${BUNDLE_PATH}" "${PLUGINS_FILES}" "${PLUGINS_PATH}")
            ]])

            find_program(CODESIGN_PATH codesign)
            if (CODESIGN_PATH)
                install(CODE "message(STATUS \"Signing bundle '${CMAKE_INSTALL_PREFIX}/${BUNDLE_NAME}'...\")")
                install(CODE "execute_process(COMMAND ${CODESIGN_PATH} --force --deep --sign - ${BUNDLE_PATH} COMMAND_ERROR_IS_FATAL ANY)")
            endif()

            install(CODE [[ message(STATUS "MacOS Bundle finalized. DO NOT TOUCH IT ANYMORE! ANY MODIFICATIONS WILL BREAK IT FROM NOW ON!") ]])
        else()
            install(TARGETS main RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
            install(TARGETS ${PLUGINS} LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins)
            install(TARGETS libfenestra LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
    else()
        install(TARGETS main RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        if (TARGET main-forwarder)
            install(TARGETS main-forwarder BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
    endif()
endfunction()