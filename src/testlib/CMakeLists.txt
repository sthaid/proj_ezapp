cmake_minimum_required(VERSION 3.16)

project(testlib VERSION 1.0 DESCRIPTION "A simple example library")

set(LIB_SRCS
    src/testlib.c
)
set(LIB_HEADERS
    include/testlib.h
)

add_library(testlib STATIC ${LIB_SRCS} ${LIB_HEADERS})

target_include_directories(testlib PUBLIC include)

install(FILES include/testlib.h DESTINATION include/testlib)

install(TARGETS testlib ARCHIVE DESTINATION lib/static)

