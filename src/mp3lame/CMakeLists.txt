cmake_minimum_required(VERSION 3.16)

project(mp3lame)

set(CMAKE_C_FLAGS "-DHAVE_CONFIG_H -O2 -Wall -g")

set(LIB_SRCS
    bitstream.c
    encoder.c
    fft.c
    gain_analysis.c
    id3tag.c
    lame.c
    mpglib_interface.c
    newmdct.c
    presets.c
    psymodel.c
    quantize.c
    quantize_pvt.c
    reservoir.c
    set_get.c
    tables.c
    takehiro.c
    util.c
    vbrquantize.c
    VbrTag.c
    version.c
)

set(LIB_HEADERS
    include/lame.h
)

add_library(mp3lame STATIC ${LIB_SRCS} ${LIB_HEADERS})

target_include_directories(mp3lame PUBLIC include PRIVATE .)

install(FILES include/lame.h DESTINATION include)

install(TARGETS mp3lame ARCHIVE DESTINATION lib)

