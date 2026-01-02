cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_FLAGS "-DHAVE_CONFIG_H -O2")

project(libmp3lame VERSION 1.0 DESCRIPTION "port of lame mp3 encoder")

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
    ../include/lame.h
)

add_library(libmp3lame STATIC ${LIB_SRCS} ${LIB_HEADERS})

target_include_directories(libmp3lame PUBLIC ../include .)

install(FILES ../include/lame.h DESTINATION include/lame)

install(TARGETS libmp3lame ARCHIVE DESTINATION lib)

