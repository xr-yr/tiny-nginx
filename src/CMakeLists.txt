add_subdirectory(app)
add_subdirectory(logic)
add_subdirectory(misc)
add_subdirectory(net)
add_subdirectory(proc)
add_subdirectory(signal)

add_library(nginx_lib INTERFACE)
target_link_libraries(nginx_lib
        INTERFACE
        app_lib
        logic_lib
        misc_lib
        net_lib
        proc_lib
        signal_lib
)
