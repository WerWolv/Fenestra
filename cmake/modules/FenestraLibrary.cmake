macro(add_fenestra_library)
    add_library(${ARGN})

    target_link_libraries(${ARGV0} PRIVATE libfenestra)
    target_compile_definitions(${ARGV0} PRIVATE FENESTRA_PROJECT_NAME="${ARG0}")
    set_target_properties(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FENESTRA_MAIN_OUTPUT_DIRECTORY})
endmacro()