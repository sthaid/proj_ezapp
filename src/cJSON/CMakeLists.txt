cmake_minimum_required(VERSION 3.16)

project(cJSON)

set(CMAKE_C_FLAGS "-O2 -Wall -Werror -g")

set(LIB_SRCS
    cJSON.c
)

set(LIB_HEADERS
    cJSON.h
)

add_library(cJSON STATIC ${LIB_SRCS} ${LIB_HEADERS})

target_include_directories(cJSON PUBLIC .)

install(FILES ./cJSON.h DESTINATION include/cJSON)

install(TARGETS cJSON ARCHIVE DESTINATION lib)

