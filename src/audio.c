/*
 * audio.c - Audio subsystem using c-flod FC14 player
 *
 * Platform-independent: uses g_audio callbacks from platform.h
 * instead of calling SDL directly.
 */

#include "audio.h"
#include "platform.h"
#include "platform_api.h"
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
static int audio_running;

/* Access CorePlayer through the inheritance chain:
 * FCPlayer -> AmigaPlayer -> CorePlayer
 * fc_player.super.super is the CorePlayer */
#define CORE_PLAYER (fc_player.super.super)
#define CORE_MIXER (amiga.super)

static void audio_fill(void* userdata, uint8_t* stream, int len)
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
        platform_log(PLATFORM_LOG_ERROR, "audio: failed to load FC14 module");
        return -1;
    }

    platform_log(PLATFORM_LOG_INFO, "audio: loaded FC14 module (version %d)", CORE_PLAYER.version);

    /* Initialize player */
    CORE_PLAYER.initialize(&CORE_PLAYER);

    /* Open audio device via platform backend */
    if (g_audio.open(44100, 2, 2048, audio_fill, NULL) < 0) {
        platform_log(PLATFORM_LOG_ERROR, "audio: failed to open audio device");
        return -1;
    }

    /* Start playback */
    g_audio.pause(0);
    audio_running = 1;

    return 0;
}

void audio_shutdown(void)
{
    if (audio_running) {
        g_audio.pause(1);
        g_audio.close();
        audio_running = 0;
    }
}
