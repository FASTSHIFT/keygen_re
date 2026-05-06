/*
 * platform.h - Platform abstraction layer
 *
 * Core logic interacts with the outside world exclusively through these
 * callback tables. The concrete backend (SDL2, Win32, headless, …) fills
 * them at startup.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

/* --- Audio backend --- */
typedef void (*audio_fill_fn)(void* userdata, uint8_t* stream, int len);

typedef struct {
    int (*open)(int freq, int channels, int samples,
        audio_fill_fn fill_cb, void* userdata);
    void (*close)(void);
    void (*pause)(int pause_on);
} PlatformAudio;

/* --- Video backend --- */
typedef struct {
    int (*open)(const char* title, int width, int height);
    void (*present)(const uint32_t* pixels, int width, int height);
    void (*close)(void);
} PlatformVideo;

/* --- Input events --- */
typedef enum {
    EVT_NONE = 0,
    EVT_QUIT,
    EVT_MOUSE_DOWN,
    EVT_KEY_DOWN,
    EVT_TEXT_INPUT,
} EventType;

typedef struct {
    EventType type;
    union {
        struct { int x, y, button; } mouse;
        struct { int sym; } key;
        struct { char text[32]; } text;
    };
} PlatformEvent;

/* Key constants (SDL-independent) */
#define PKEY_ESCAPE 0x1B
#define PKEY_BACKSPACE 0x08

typedef struct {
    int (*poll)(PlatformEvent* event); /* returns 1 if event available */
    void (*delay_ms)(int ms);
} PlatformInput;

/* --- Global backend instances (assigned by main at startup) --- */
extern PlatformAudio g_audio;
extern PlatformVideo g_video;
extern PlatformInput g_input;

#endif /* PLATFORM_H */
