cmake_minimum_required(VERSION 3.16)
project(keygen_re C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# c-flod's Common.h hardcodes INT3 as the x86 `int3` instruction, which the
# ARM64 (Apple Silicon) assembler doesn't recognize. Patch the submodule
# header in-place at configure time to make it architecture-aware. Idempotent:
# skips if already patched.
set(_cflod_common_h "${CMAKE_CURRENT_SOURCE_DIR}/c-flod/flashlib/Common.h")
if(EXISTS "${_cflod_common_h}")
    file(READ "${_cflod_common_h}" _cflod_common_contents)
    if(NOT _cflod_common_contents MATCHES "__i386__")
        string(REPLACE
            "#define INT3 __asm__(\"int3\")"
            "#if defined(__i386__) || defined(__x86_64__)\n#define INT3 __asm__(\"int3\")\n#else\n#define INT3 ((void)0)\n#endif"
            _cflod_common_patched "${_cflod_common_contents}")
        file(WRITE "${_cflod_common_h}" "${_cflod_common_patched}")
        message(STATUS "Patched c-flod/flashlib/Common.h: made INT3 portable across architectures")
    endif()
endif()

# --- Third-party: c-flod (no -Werror, suppress noisy warnings) ---
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

target_compile_options(cflod PRIVATE -w -Wno-incompatible-function-pointer-types)

# --- Our sources: strict warnings + -Werror ---
add_executable(keygen_re
    src/main.c
    src/keygen.c
    src/audio.c
    src/render.c
)

target_include_directories(keygen_re PRIVATE
    ${SDL2_INCLUDE_DIRS}
    src
    src/resources
)

target_link_libraries(keygen_re
    cflod
    ${SDL2_LIBRARIES}
    m
)

target_link_directories(keygen_re PRIVATE ${SDL2_LIBRARY_DIRS})

target_compile_options(keygen_re PRIVATE
    ${SDL2_CFLAGS_OTHER}
    -Wall -Werror
    -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
    -Wno-unknown-pragmas
)
