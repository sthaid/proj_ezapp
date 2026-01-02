cmake_minimum_required(VERSION 3.16)

project(cJSON VERSION 1.0 DESCRIPTION "cJSON port")

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

