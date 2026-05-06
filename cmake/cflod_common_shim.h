/*
 * Portable replacement for c-flod/flashlib/Common.h.
 *
 * This file is force-included (via -include) into every translation unit of
 * the `cflod` target. By defining the COMMON_H header guard up front, the
 * original submodule header is short-circuited whenever it gets pulled in
 * transitively, so we don't have to touch any file inside c-flod/.
 *
 * Changes vs. upstream:
 *   1. INT3 is architecture-aware (ARM64 stubs it out).
 *   2. DEBUGP and fprintf/malloc/free are redirected through platform_api
 *      hooks so the entire cflod library is MCU-portable without libc stdio.
 */
#ifndef COMMON_H
#define COMMON_H

#include <math.h>
#include <stddef.h>
#include <stdbool.h>

/* Pull in platform API for logging and memory */
#include "platform_api.h"

#define null NULL

typedef float Number;

/*
 * Redirect c-flod's debug prints through platform logging.
 * On MCU this goes to UART/RTT; on hosted platforms to stderr.
 */
#define DEBUGP(format, ...) platform_log(PLATFORM_LOG_DEBUG, format, __VA_ARGS__)

/*
 * c-flod uses fprintf(stderr, ...) for error diagnostics in ByteArray.c.
 * We macro-redirect these to platform_log so no stdio is required at link time.
 * Note: c-flod never uses fprintf for anything other than stderr diagnostics.
 */
#include <stdio.h>  /* still needed for FILE* type in case headers reference it */
#define fprintf(stream, ...) (platform_log(PLATFORM_LOG_WARN, __VA_ARGS__), 0)
#define perror(msg) platform_log(PLATFORM_LOG_ERROR, "%s", (msg))

/*
 * Redirect malloc/free through platform API so MCU targets can use a static
 * pool. ByteArray_new() is the only caller and is currently unused, but we
 * cover it for completeness.
 */
#include <stdlib.h>  /* for size_t, original code expects stdlib symbols */
#undef malloc
#undef free
#define malloc(sz) platform_malloc(sz)
#define free(ptr) platform_free(ptr)

#if defined(__i386__) || defined(__x86_64__)
#  define INT3 __asm__("int3")
#else
#  define INT3 ((void)0)
#endif

#endif /* COMMON_H */
