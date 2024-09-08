project(trace)

add_library(trace OBJECT
    source/stacktrace.cpp
    source/exceptions.cpp
)
target_include_directories(trace PUBLIC include)

find_library(HAS_LIBSTDCXX_EXP stdc++exp)
if (HAS_LIBSTDCXX_EXP)
    target_link_libraries(trace PRIVATE stdc++exp)
endif()

if (NOT FENESTRA_DISABLE_STACKTRACE)
    if (WIN32)
        message(STATUS "StackWalk enabled!")
        target_link_libraries(trace PRIVATE DbgHelp.lib)
    else ()
        find_package(Backtrace)
        if (${Backtrace_FOUND})
            message(STATUS "Backtrace enabled! Header: ${Backtrace_HEADER}")

            if (Backtrace_HEADER STREQUAL "backtrace.h")
                target_link_libraries(trace PRIVATE ${Backtrace_LIBRARY})
                target_include_directories(trace PRIVATE ${Backtrace_INCLUDE_DIR})
                target_compile_definitions(trace PRIVATE BACKTRACE_HEADER=<${Backtrace_HEADER}>)
                target_compile_definitions(trace PRIVATE FENESTRA_HAS_BACKTRACE)
            elseif (Backtrace_HEADER STREQUAL "execinfo.h")
                target_link_libraries(trace PRIVATE  ${Backtrace_LIBRARY})
                target_include_directories(trace PRIVATE ${Backtrace_INCLUDE_DIR})
                target_compile_definitions(trace PRIVATE BACKTRACE_HEADER=<${Backtrace_HEADER}>)
                target_compile_definitions(trace PRIVATE FENESTRA_HAS_EXECINFO)
            endif()
        endif()
    endif()
endif()

if (NOT FENESTRA_NO_TRACE_EXCEPTIONS)
    if (HAS_WRAP_CXA_THROW)
        target_link_options(trace PUBLIC "-Wl,--wrap=__cxa_throw")
        target_compile_definitions(trace PRIVATE FENESTRA_WRAP_CXA_THROW=1)
    endif()
endif()