cmake_minimum_required(VERSION 3.16)
project(keygen_re C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# --- Third-party: c-flod (no -Werror, suppress noisy warnings) ---
# We don't modify anything inside the c-flod submodule. Instead, a shim header
# (cmake/cflod_common_shim.h) is force-included ahead of the original
# flashlib/Common.h; its COMMON_H header guard then suppresses the upstream
# copy entirely. This keeps the submodule pristine while fixing the x86-only
# INT3 macro on ARM64.
add_library(cflod STATIC
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

target_include_directories(cflod PUBLIC
    c-flod
    c-flod/include
    c-flod/flashlib
    c-flod/neoart/flod
    c-flod/neoart/flod/core
)

target_compile_options(cflod PRIVATE
    -w -Wno-incompatible-function-pointer-types
    -include "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cflod_common_shim.h"
)

# --- Platform backend: SDL2 (the ONLY target that links SDL) ---
add_library(platform_sdl2 STATIC src/platform_sdl2.c)

target_include_directories(platform_sdl2 PRIVATE
    ${SDL2_INCLUDE_DIRS}
    src
)

target_link_libraries(platform_sdl2 PRIVATE ${SDL2_LIBRARIES})
target_link_directories(platform_sdl2 PRIVATE ${SDL2_LIBRARY_DIRS})

target_compile_options(platform_sdl2 PRIVATE
    ${SDL2_CFLAGS_OTHER}
    -Wall -Werror
    -Wno-unused-parameter
)

# --- Our sources: strict warnings + -Werror, NO SDL dependency ---
add_executable(keygen_re
    src/main.c
    src/keygen.c
    src/audio.c
    src/render.c
)

target_include_directories(keygen_re PRIVATE
    src
    src/resources
)

target_link_libraries(keygen_re
    cflod
    platform_sdl2
    ${SDL2_LIBRARIES}
    m
)

target_link_directories(keygen_re PRIVATE ${SDL2_LIBRARY_DIRS})

target_compile_options(keygen_re PRIVATE
    -Wall -Werror
    -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
    -Wno-unknown-pragmas
)
