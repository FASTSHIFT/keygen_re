/*
 * audio.c - Audio subsystem using c-flod FC14 player + SDL2
 */

#include "audio.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

/* c-flod headers */
#include "../c-flod/flashlib/ByteArray.h"
#include "../c-flod/neoart/flod/core/Amiga.h"
#include "../c-flod/neoart/flod/core/CoreMixer.h"
#include "../c-flod/neoart/flod/core/CorePlayer.h"
#include "../c-flod/neoart/flod/futurecomposer/FCPlayer.h"

static struct Amiga amiga;
static struct FCPlayer fc_player;
static struct ByteArray wave_stream;
static char wave_buffer[8192]; /* c-flod expects char* */
static SDL_AudioDeviceID audio_dev;
static int audio_running;

/* Access CorePlayer through the inheritance chain:
 * FCPlayer -> AmigaPlayer -> CorePlayer
 * fc_player.super.super is the CorePlayer */
#define CORE_PLAYER (fc_player.super.super)
#define CORE_MIXER (amiga.super)

static void audio_callback(void* userdata, Uint8* stream, int len)
{
    (void)userdata;
    int filled = 0;

    while (filled < len) {
        wave_stream.pos = 0;
        CORE_MIXER.accurate(&CORE_MIXER);

        int avail = wave_stream.pos;
        if (avail <= 0) {
            memset(stream + filled, 0, len - filled);
            return;
        }

        int to_copy = avail;
        if (filled + to_copy > len)
            to_copy = len - filled;

        memcpy(stream + filled, wave_buffer, to_copy);
        filled += to_copy;
    }
}

int audio_init(const uint8_t* tune_data, int tune_size)
{
    SDL_AudioSpec want, have;
    struct ByteArray tune_stream;

    /* Initialize c-flod Amiga hardware emulation */
    Amiga_ctor(&amiga);

    /* Set up wave output stream */
    ByteArray_ctor(&wave_stream);
    wave_stream.endian = BAE_LITTLE;
    ByteArray_open_mem(&wave_stream, wave_buffer, sizeof(wave_buffer));
    CORE_MIXER.wave = &wave_stream;

    /* Create FC player */
    FCPlayer_ctor(&fc_player, &amiga);

    /* Load tune data */
    ByteArray_ctor(&tune_stream);
    tune_stream.endian = BAE_BIG;
    ByteArray_open_mem(&tune_stream, (char*)tune_data, tune_size);

    CorePlayer_load(&CORE_PLAYER, &tune_stream);
    if (!CORE_PLAYER.version) {
        fprintf(stderr, "audio: failed to load FC14 module\n");
        return -1;
    }

    printf("audio: loaded FC14 module (version %d)\n", CORE_PLAYER.version);

    /* Initialize player */
    CORE_PLAYER.initialize(&CORE_PLAYER);

    /* Open SDL audio */
    memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 2048;
    want.callback = audio_callback;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_dev == 0) {
        fprintf(stderr, "audio: SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return -1;
    }

    printf("audio: opened device (freq=%d, channels=%d, samples=%d)\n",
        have.freq, have.channels, have.samples);

    /* Start playback */
    SDL_PauseAudioDevice(audio_dev, 0);
    audio_running = 1;

    return 0;
}

void audio_shutdown(void)
{
    if (audio_running) {
        SDL_PauseAudioDevice(audio_dev, 1);
        SDL_CloseAudioDevice(audio_dev);
        audio_running = 0;
    }
}
