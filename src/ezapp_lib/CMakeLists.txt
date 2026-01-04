cmake_minimum_required(VERSION 3.16)

project(ezapp_lib)

set(CMAKE_C_FLAGS "-O2 -Wall -Werror -Wno-unused-result -g")

set(LIB_SRCS
    sdlx_audio.c
    sdlx_event.c
    sdlx_misc.c
    sdlx_sensor.c
    sdlx_video.c
    svcs.c
    utils.c
    utils_android.cpp
    logging.c
    run.c
)

set(LIB_HEADERS
    include/sdlx.h
    include/svcs.h
    include/utils.h
    include/run.h
    include/logging.h
)

set(INCLUDE_PATH "")
cmake_path(APPEND INCLUDE_PATH "${CMAKE_INSTALL_PREFIX}" include)

include_directories(${INCLUDE_PATH} . ./include)

add_library(ezapp_lib STATIC ${LIB_SRCS} ${LIB_HEADERS})

install(FILES include/sdlx.h include/svcs.h include/utils.h include/run.h include/logging.h
        DESTINATION include)

install(TARGETS ezapp_lib ARCHIVE DESTINATION lib)

