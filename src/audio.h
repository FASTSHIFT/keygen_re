/*
 * audio.h - Audio subsystem using c-flod FC14 player + SDL2
 */
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/* Initialize audio: load FC14 tune and start playback */
int audio_init(const uint8_t* tune_data, int tune_size);

/* Shutdown audio */
void audio_shutdown(void);

#endif /* AUDIO_H */
