/*
 * platform_api_hosted.c - Default platform API for hosted environments
 *
 * Uses stdio for logging and stdlib for memory allocation.
 * Link this file on Linux/macOS/Windows. On MCU targets, provide
 * your own platform_log / platform_malloc / platform_free instead.
 */

#include "platform_api.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * Logging
 * ============================================================ */

void platform_log(PlatformLogLevel level, const char* fmt, ...)
{
    FILE* out = (level >= PLATFORM_LOG_WARN) ? stderr : stdout;
    static const char* prefixes[] = { "[DBG] ", "[INF] ", "[WRN] ", "[ERR] " };

    if (level >= PLATFORM_LOG_DEBUG && level <= PLATFORM_LOG_ERROR)
        fputs(prefixes[level], out);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);

    fputc('\n', out);
}

/* ============================================================
 * Memory allocation
 * ============================================================ */

void* platform_malloc(size_t size)
{
    return malloc(size);
}

void platform_free(void* ptr)
{
    free(ptr);
}
