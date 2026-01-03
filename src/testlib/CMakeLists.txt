cmake_minimum_required(VERSION 3.16)

project(testlib)

set(CMAKE_C_FLAGS "-O2 -Wall -Werror -g")

set(LIB_SRCS
    src/testlib.c
)
set(LIB_HEADERS
    include/testlib.h
)

add_library(testlib STATIC ${LIB_SRCS} ${LIB_HEADERS})

target_include_directories(testlib PUBLIC include)

install(FILES include/testlib.h DESTINATION include/testlib)

install(TARGETS testlib ARCHIVE DESTINATION lib)

