project(main-forwarder)

add_executable(main-forwarder
        source/main.cpp
)

target_link_libraries(main-forwarder PRIVATE wolv::io fmt::fmt)
add_dependencies(fenestra_all main-forwarder)
set_target_properties(main-forwarder PROPERTIES
        OUTPUT_NAME ${FENESTRA_APPLICATION_NAME_LOWER}
        RUNTIME_OUTPUT_DIRECTORY "${FENESTRA_MAIN_OUTPUT_DIRECTORY}"
)