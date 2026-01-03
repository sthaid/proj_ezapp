cmake_minimum_required(VERSION 3.16)

project(ezapp)

set(CMAKE_C_FLAGS "-O2 -Wall -Wno-unused-result -Werror -g")

set(INCLUDE_PATH "")
cmake_path(APPEND INCLUDE_PATH "${CMAKE_PREFIX_PATH}" include)

set(LIB_SDL "")
cmake_path(APPEND LIB_SDL "${CMAKE_PREFIX_PATH}" lib/libSDL3.so)
set(LIB_SDL_TTF "")
cmake_path(APPEND LIB_SDL_TTF "${CMAKE_PREFIX_PATH}" lib/libSDL3_ttf.so)
set(LIB_SDL_MIXER "")
cmake_path(APPEND LIB_SDL_MIXER "${CMAKE_PREFIX_PATH}" lib/libSDL3_mixer.so)
set(LIB_MP3LAME "")
cmake_path(APPEND LIB_MP3LAME "${CMAKE_PREFIX_PATH}" lib/libmp3lame.a)
set(LIB_CJSON "")
cmake_path(APPEND LIB_CJSON "${CMAKE_PREFIX_PATH}" lib/libcJSON.a)
set(LIB_PICOC "")
cmake_path(APPEND LIB_PICOC "${CMAKE_PREFIX_PATH}" lib/libpicoc.a)

include_directories(${INCLUDE_PATH} .)

add_executable(ezapp 
    logging.c
    main.c
    sdlx_audio.c
    sdlx_event.c
    sdlx_misc.c
    sdlx_sensor.c
    sdlx_video.c
    svcs.c
    utils.c
    utils_android.cpp
        )

target_link_libraries(ezapp PRIVATE 
    ${LIB_SDL} ${LIB_SDL_TTF} ${LIB_SDL_MIXER} 
    ${LIB_PICOC} ${LIB_MP3LAME} ${LIB_CJSON} 
    m)

