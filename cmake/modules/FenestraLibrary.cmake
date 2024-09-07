macro(add_fenestra_library)
    add_library(${ARGN})

    target_link_libraries(${ARG0} PRIVATE libfenestra)
    target_compile_definitions(${ARG0} PRIVATE FENESTRA_PROJECT_NAME="${ARG0}")
endmacro()