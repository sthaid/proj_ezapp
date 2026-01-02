cmake_minimum_required(VERSION 3.16)
project(testprog)

set(INCLUDE_PATH "")
cmake_path(APPEND INCLUDE_PATH "${CMAKE_PREFIX_PATH}" include)

set(LIB_PATH "")
cmake_path(APPEND LIB_PATH "${CMAKE_PREFIX_PATH}" lib/libtestlib.a)

include_directories(${INCLUDE_PATH})

add_executable(testprog main.c)

target_link_libraries(testprog PRIVATE ${LIB_PATH})

