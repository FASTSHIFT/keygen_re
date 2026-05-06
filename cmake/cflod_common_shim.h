/*
 * Portable replacement for c-flod/flashlib/Common.h.
 *
 * This file is force-included (via -include) into every translation unit of
 * the `cflod` target. By defining the COMMON_H header guard up front, the
 * original submodule header is short-circuited whenever it gets pulled in
 * transitively, so we don't have to touch any file inside c-flod/.
 *
 * The only functional change vs. upstream is making INT3 architecture-aware:
 * AppleClang / ARM64 rejects the raw x86 `int3` mnemonic.
 */
#ifndef COMMON_H
#define COMMON_H

#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#define null NULL

typedef float Number;

#define DEBUGP(format, ...) fprintf(stderr, format, __VA_ARGS__)

#if defined(__i386__) || defined(__x86_64__)
#  define INT3 __asm__("int3")
#else
#  define INT3 ((void)0)
#endif

#endif /* COMMON_H */
