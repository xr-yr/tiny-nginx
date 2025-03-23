cmake_minimum_required(VERSION 3.30)
project(tiny_nginx)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
add_subdirectory(src)

add_executable(tiny-nginx src/main.cpp)
target_link_libraries(tiny-nginx PRIVATE nginx_lib)
