cmake_minimum_required(VERSION 3.16)
project(edge-demoscene-sdl2 C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# c-flod FC14 player sources (core + futurecomposer only)
set(CFLOD_SOURCES
    c-flod/flashlib/ByteArray.c
    c-flod/neoart/flod/core/Amiga.c
    c-flod/neoart/flod/core/AmigaChannel.c
    c-flod/neoart/flod/core/AmigaFilter.c
    c-flod/neoart/flod/core/AmigaPlayer.c
    c-flod/neoart/flod/core/AmigaRow.c
    c-flod/neoart/flod/core/AmigaSample.c
    c-flod/neoart/flod/core/AmigaStep.c
    c-flod/neoart/flod/core/CoreMixer.c
    c-flod/neoart/flod/core/CorePlayer.c
    c-flod/neoart/flod/core/Sample.c
    c-flod/neoart/flod/core/SBChannel.c
    c-flod/neoart/flod/core/SBPlayer.c
    c-flod/neoart/flod/core/SBSample.c
    c-flod/neoart/flod/core/Soundblaster.c
    c-flod/neoart/flod/futurecomposer/FCPlayer.c
    c-flod/neoart/flod/futurecomposer/FCVoice.c
)

add_executable(edge-demo
    src/main.c
    src/keygen.c
    src/audio.c
    src/render.c
    ${CFLOD_SOURCES}
)

target_include_directories(edge-demo PRIVATE
    ${SDL2_INCLUDE_DIRS}
    src
    src/resources
    c-flod
    c-flod/include
    c-flod/flashlib
    c-flod/neoart/flod
    c-flod/neoart/flod/core
)

target_link_libraries(edge-demo
    ${SDL2_LIBRARIES}
    m
)

target_compile_options(edge-demo PRIVATE
    ${SDL2_CFLAGS_OTHER}
    -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
)
