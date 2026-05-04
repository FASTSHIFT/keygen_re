/*
 * fc14player.h - Future Composer 1.4 player
 * Ported from IDA decompilation of keygen_new2032.exe
 *
 * Original functions:
 *   sub_803C30 -> fc14_init
 *   sub_804240 -> fc14_tick
 *   sub_804340 -> fc14_process_channel
 *   sub_804530 -> fc14_process_effects
 *   sub_804930 -> fc14_process_envelope
 *   sub_804BA0 -> fc14_set_sample
 *   sub_804BF0 -> fc14_restart_sample
 *   sub_804C00 -> fc14_set_frequency
 *   sub_804C70 -> fc14_setup_mixer
 *   sub_804E30 -> fc14_render_audio
 *   sub_804F60 -> fc14_mix_mono8
 */
#ifndef FC14PLAYER_H
#define FC14PLAYER_H

#include <stdint.h>

/* Initialize FC14 player with module data */
int fc14_init(const uint8_t* data, int size, int16_t start_pos);

/* Setup mixer for given sample rate */
void fc14_setup_mixer(uint32_t sample_rate, int bits, int channels, int16_t interp);

/* Render audio samples into buffer */
void fc14_render_audio(uint8_t* buffer, uint32_t num_samples);

/* Get format description string */
const char* fc14_get_format(void);

#endif /* FC14PLAYER_H */
