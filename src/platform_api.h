/*
 * platform_api.h - Platform API (portable primitives)
 *
 * Provides replaceable hooks for:
 *   - Logging (printf/fprintf replacement)
 *   - Memory allocation (malloc/free replacement)
 *
 * On hosted platforms (Linux/macOS/Windows) the default implementations
 * use stdio + stdlib. On MCU targets, link your own platform_xxx functions
 * or assign custom callbacks at startup.
 *
 * Usage:
 *   #include "platform_api.h"
 *   platform_log(PLATFORM_LOG_INFO, "loaded module version %d", ver);
 *   void* p = platform_malloc(1024);
 *   platform_free(p);
 */
#ifndef PLATFORM_API_H
#define PLATFORM_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Logging
 * ============================================================ */

typedef enum {
    PLATFORM_LOG_DEBUG = 0,
    PLATFORM_LOG_INFO,
    PLATFORM_LOG_WARN,
    PLATFORM_LOG_ERROR,
} PlatformLogLevel;

/*
 * Log a message. The default implementation prints to stdout/stderr.
 * MCU ports can redirect to UART, RTT, semihosting, or /dev/null.
 */
void platform_log(PlatformLogLevel level, const char* fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* ============================================================
 * Memory allocation
 * ============================================================ */

/*
 * Drop-in replacements for malloc/free.
 * Default: forward to libc. MCU ports can use a static pool, FreeRTOS
 * pvPortMalloc, or a bump allocator.
 */
void* platform_malloc(size_t size);
void platform_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_API_H */
