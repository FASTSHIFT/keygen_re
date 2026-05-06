/*
 * platform_sdl2.c - SDL2 backend implementation
 *
 * This is the ONLY file that includes <SDL.h>. All platform-specific
 * calls are concentrated here behind the PlatformAudio/Video/Input
 * interfaces defined in platform.h.
 */

#include "platform.h"
#include <SDL.h>
#include <string.h>

/* ============================================================
 * Audio
 * ============================================================ */

static SDL_AudioDeviceID s_audio_dev;

static int sdl_audio_open(int freq, int channels, int samples,
    audio_fill_fn fill_cb, void* userdata)
{
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = freq;
    want.format = AUDIO_S16SYS;
    want.channels = (uint8_t)channels;
    want.samples = (uint16_t)samples;
    want.callback = (SDL_AudioCallback)fill_cb;
    want.userdata = userdata;

    s_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (s_audio_dev == 0)
        return -1;
    return 0;
}

static void sdl_audio_close(void)
{
    if (s_audio_dev) {
        SDL_CloseAudioDevice(s_audio_dev);
        s_audio_dev = 0;
    }
}

static void sdl_audio_pause(int pause_on)
{
    if (s_audio_dev)
        SDL_PauseAudioDevice(s_audio_dev, pause_on);
}

/* ============================================================
 * Video
 * ============================================================ */

static SDL_Window* s_win;
static SDL_Renderer* s_ren;
static SDL_Texture* s_tex;

static int sdl_video_open(const char* title, int w, int h)
{
    s_win = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, SDL_WINDOW_SHOWN);
    if (!s_win)
        return -1;

    s_ren = SDL_CreateRenderer(s_win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_ren)
        s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_SOFTWARE);
    if (!s_ren)
        return -1;

    s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!s_tex)
        return -1;

    return 0;
}

static void sdl_video_present(const uint32_t* pixels, int w, int h)
{
    (void)h;
    SDL_UpdateTexture(s_tex, NULL, pixels, w * (int)sizeof(uint32_t));
    SDL_RenderClear(s_ren);
    SDL_RenderCopy(s_ren, s_tex, NULL, NULL);
    SDL_RenderPresent(s_ren);
}

static void sdl_video_close(void)
{
    if (s_tex) { SDL_DestroyTexture(s_tex); s_tex = NULL; }
    if (s_ren) { SDL_DestroyRenderer(s_ren); s_ren = NULL; }
    if (s_win) { SDL_DestroyWindow(s_win); s_win = NULL; }
}

/* ============================================================
 * Input
 * ============================================================ */

static int sdl_input_poll(PlatformEvent* ev)
{
    SDL_Event e;
    if (!SDL_PollEvent(&e))
        return 0;

    ev->type = EVT_NONE;
    switch (e.type) {
    case SDL_QUIT:
        ev->type = EVT_QUIT;
        break;
    case SDL_MOUSEBUTTONDOWN:
        ev->type = EVT_MOUSE_DOWN;
        ev->mouse.x = e.button.x;
        ev->mouse.y = e.button.y;
        ev->mouse.button = e.button.button;
        break;
    case SDL_KEYDOWN:
        ev->type = EVT_KEY_DOWN;
        if (e.key.keysym.sym == SDLK_ESCAPE)
            ev->key.sym = PKEY_ESCAPE;
        else if (e.key.keysym.sym == SDLK_BACKSPACE)
            ev->key.sym = PKEY_BACKSPACE;
        else
            ev->key.sym = e.key.keysym.sym;
        break;
    case SDL_TEXTINPUT:
        ev->type = EVT_TEXT_INPUT;
        memcpy(ev->text.text, e.text.text, sizeof(ev->text.text));
        break;
    default:
        return 0; /* unknown event, report as "no event" */
    }
    return 1;
}

static void sdl_input_delay(int ms)
{
    SDL_Delay((uint32_t)ms);
}

/* ============================================================
 * Global backend tables
 * ============================================================ */

PlatformAudio g_audio = { sdl_audio_open, sdl_audio_close, sdl_audio_pause };
PlatformVideo g_video = { sdl_video_open, sdl_video_present, sdl_video_close };
PlatformInput g_input = { sdl_input_poll, sdl_input_delay };

/* ============================================================
 * Lifecycle helpers
 * ============================================================ */

void platform_sdl2_init(void)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

void platform_sdl2_quit(void)
{
    SDL_Quit();
}
