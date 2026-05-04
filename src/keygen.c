/*
 * keygen.c - Keil license key generation (STUB)
 *
 * NOTICE: The original keygen algorithm has been intentionally removed.
 * This project is open-sourced for technical exchange purposes only,
 * focusing on demoscene rendering, Amiga FC14 music playback, and
 * Win32-to-SDL2 porting techniques.
 *
 * The function signatures are preserved to keep the UI workflow intact.
 * All "generated" output is purely random and non-functional.
 *
 * Original functions (addresses from keygen_new2032.exe):
 *   sub_803A70 -> mt_seed
 *   sub_803AD0 -> mt_rand
 *   sub_802DE0 -> encode_base35
 *   sub_8039E0 -> decode_base35
 *   sub_803A00 -> transform_char
 *   sub_802E00 -> compute_crc
 *   sub_802ED0 -> compute_checksum
 *   sub_802900 -> generate_serial
 *   sub_8030D0 -> generate_license_key
 */

#include "keygen.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * MT19937 PRNG - kept as a general-purpose utility for the
 * rendering subsystem (star field initialization, etc.)
 * This is a standard algorithm, not part of the keygen logic.
 * ============================================================ */

static uint32_t mt_state[624];
static int mt_index = 625;

void mt_seed(uint32_t seed)
{
    uint32_t* p = mt_state;
    do {
        *p = seed & 0xFFFF0000;
        seed = seed * 0x10DCD + 1;
        *p |= seed >> 16;
        seed = seed * 0x10DCD + 1;
        p++;
    } while (p < mt_state + 624);
    mt_index = 624;
}

uint32_t mt_rand(void)
{
    static const uint32_t twist_xor[2] = { 0x00000000, 0x9908b0df };
    uint32_t y;
    int i;

    if (mt_index > 623) {
        if (mt_index == 625) {
            uint32_t seed = 0x985;
            uint32_t* p = mt_state;
            do {
                *p = seed & 0xFFFF0000;
                seed = seed * 0x10DCD + 1;
                *p |= seed >> 16;
                seed = seed * 0x10DCD + 1;
                p++;
            } while (p < mt_state + 624);
        }

        for (i = 0; i < 227; i++) {
            y = (mt_state[i] & 0x80000000) ^ ((mt_state[i + 1] ^ mt_state[i]) & 0x7FFFFFFF);
            mt_state[i] = twist_xor[y & 1] ^ (y >> 1) ^ mt_state[i + 397];
        }
        for (; i < 623; i++) {
            y = (mt_state[i] & 0x80000000) ^ ((mt_state[i + 1] ^ mt_state[i]) & 0x7FFFFFFF);
            mt_state[i] = twist_xor[y & 1] ^ (y >> 1) ^ mt_state[i - 227];
        }
        y = (mt_state[623] & 0x80000000) ^ ((mt_state[0] ^ mt_state[623]) & 0x7FFFFFFF);
        mt_state[623] = twist_xor[y & 1] ^ (y >> 1) ^ mt_state[396];

        mt_index = 0;
    }

    y = mt_state[mt_index++];

    /* Tempering */
    y ^= y >> 11;
    y ^= (y & 0xFF3A58AD) << 7;
    y ^= (y & 0xFFFFDF8C) << 15;
    y ^= y >> 18;

    return y;
}

/* ============================================================
 * Stub helpers - generate random-looking but invalid output
 * ============================================================ */

static const char base35_chars[] = "0123456789ABCDEFGHIJKLMNPQRSTUVWXYZ";

static char random_base35(void)
{
    return base35_chars[mt_rand() % 35];
}

/* ============================================================
 * generate_serial (STUB)
 *
 * Original algorithm removed. Produces random output in the
 * correct format (XXXXX-XXXXX-XXXXX-XXXXX) for UI demonstration.
 * ============================================================ */

void generate_serial(char* out, int use_cid, int target, int license)
{
    int i;
    (void)use_cid;
    (void)target;
    (void)license;

    for (i = 0; i < 23; i++)
        out[i] = random_base35();

    out[5] = '-';
    out[11] = '-';
    out[17] = '-';
    out[23] = '\0';
}

/* ============================================================
 * generate_license_key (STUB)
 *
 * Original algorithm removed. Produces random output in the
 * correct format (XXXXX-XXXXX-XXXXX-XXXXX-XXXXX-XXXXX-XXXXX)
 * for UI demonstration.
 * ============================================================ */

void generate_license_key(char* out, char* cid, int use_cid,
    int target, int license)
{
    int i;
    (void)cid;
    (void)use_cid;
    (void)target;
    (void)license;

    for (i = 0; i < 41; i++)
        out[i] = random_base35();

    out[5] = '-';
    out[11] = '-';
    out[17] = '-';
    out[23] = '-';
    out[29] = '-';
    out[35] = '-';
    out[41] = '\0';
}
