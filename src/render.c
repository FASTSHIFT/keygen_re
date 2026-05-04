/*
 * render.c - 3D rendering subsystem
 * Direct port from IDA decompilation of keygen_new2032.exe
 *
 * Original functions:
 *   sub_8016C0 -> render_textured_rect
 *   sub_801830 -> render_masked_rect
 *   sub_801940 -> render_scroll_text
 *   sub_801BB0 -> render_starfield
 *   sub_801E50 -> render_3d_logo
 *   sub_801F20 -> render thread (split into render_init + render_frame)
 */

#include "render.h"
#include "keygen.h"
#include "resources/render_data.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Constants from original binary (IDA: dbl_806xxx)
 * ============================================================ */

static const double K_INV255 = 0.00392156862745098; /* dbl_806138: 1/255 */
static const double K_DEPTH = 3800.0; /* dbl_806148 */
static const double K_HALF = 0.5; /* dbl_806150 */
static const double K_SINTAB = 4096.0; /* dbl_806168 */
static const double K_SINSCALE = 0.0007669903930664063; /* dbl_806170: ~2*PI/8192 */
static const double K_FOGSCALE = 0.09107142857142857; /* dbl_806178 */
static const double K_PROJ = 300.0; /* dbl_806180 */
static const double K_OFFSET = 1900.0; /* dbl_806188 */
static const double K_NEG1 = -1.0; /* dbl_806190 */
static const double K_POS1 = 1.0; /* dbl_806198 */
static const double K_WRAP = 2800.0; /* dbl_8061A0 */
static const double K_ZERO = 0.0; /* dbl_8061A8 */
static const double K_AMP = 4.0; /* dbl_8061B0 */

/* Phase increment speeds */
static const double K_SPD1 = 0.0039; /* dbl_8061B8 */
static const double K_SPD2 = 0.0023; /* dbl_8061C0 */
static const double K_SPD3 = 0.003; /* dbl_8061C8 */
static const double K_SPD4 = 0.00039; /* dbl_8061D0 */
static const double K_SPD5 = 0.00023; /* dbl_8061D8 */
static const double K_SPD6 = 0.0003; /* dbl_8061E0 */

/* ============================================================
 * Global state
 * ============================================================ */

static uint32_t framebuffer[RENDER_WIDTH * RENDER_HEIGHT];

#define NUM_STARS 2800
static double star_x[NUM_STARS];
static double star_y[NUM_STARS];
static double star_z[NUM_STARS];

static double phase1, phase2, phase3, phase4, phase5, phase6;
static int frame_counter;
static int scroll_x;
static int scroll_char;

/* Scrolling text characters - now in render_data.h */

/* Logo pattern - now in render_data.h */

/* ============================================================
 * sub_8016C0 - Draw textured rectangle (for 3D logo)
 * Samples 4 bytes from texture_lut to form BGRA pixel
 * ============================================================ */

static void render_textured_rect(int a1, int a2, int a3, int a4,
    int a5, int a6, int a7)
{
    /* sub_8016C0: direct port from IDA */
    int idx, v9, v10, v16, i;
    float v15 = (float)a7;
    double v11, v12;
    uint16_t v13;
    uint32_t v14;

    idx = a1 + RENDER_WIDTH * a2;
    if (a1 >= RENDER_WIDTH)
        return;

    v9 = a3;
    if (a1 + a3 <= 0 || a2 >= RENDER_HEIGHT || a2 + a4 <= 0)
        return;

    if (a1 + a3 > RENDER_WIDTH)
        v9 = RENDER_WIDTH - a1;
    if (a2 + a4 > RENDER_HEIGHT)
        a4 = RENDER_HEIGHT - a2;

    for (i = 0; i < a4; i++) {
        v10 = 0;
        v16 = 0;
        if (v9 > 0) {
            v11 = (double)(int)(int64_t)(((double)i + (double)a6) * 4.0f) * v15;
            do {
                v12 = (double)(int)(int64_t)(((double)v16 + (double)a5) * 4.0f) + v11;
                int ti = (int)(int64_t)v12;
                /* Bounds check on texture_lut */
                if (ti >= 0 && ti + 3 < (int)sizeof(texture_lut)) {
                    uint8_t b0 = texture_lut[ti];
                    uint8_t b1 = texture_lut[(int)(int64_t)(1.0f + v12)];
                    uint8_t b2 = texture_lut[(int)(int64_t)(v12 + 3.0f)];
                    uint8_t b3 = texture_lut[(int)(int64_t)(2.0f + v12)];
                    /* Original: v14 = b2 | ((b3 | ((b0<<8 | b1) << 8)) << 8) */
                    v14 = (uint32_t)b2 | (((uint32_t)b3 | (((uint32_t)b0 << 8 | (uint32_t)b1) << 8)) << 8);
                    if (v14 && (idx + v10) >= 0 && (idx + v10) < RENDER_WIDTH * RENDER_HEIGHT)
                        framebuffer[idx + v10] = v14;
                }
                v16 = ++v10;
            } while (v10 < v9);
        }
        idx += RENDER_WIDTH;
    }
}

/* ============================================================
 * sub_801830 - Draw masked rectangle (for status bar text)
 * Checks font_bitmap[index] for non-zero to draw pixel
 * ============================================================ */

static void render_masked_rect(int a1, int a2, int a3, int a4,
    int a5, int a6, int a7,
    int a8, uint32_t a9)
{
    /* sub_801830: direct port from IDA */
    int idx, v11, v12, v13, v16, i;
    float v15 = (float)a7;
    double v14;

    idx = a1 + RENDER_WIDTH * a2;
    if (a1 >= RENDER_WIDTH)
        return;

    v11 = a3;
    if (a1 + a3 <= 0 || a2 >= RENDER_HEIGHT)
        return;
    v12 = a4;
    if (a2 + a4 <= 0)
        return;

    if (a1 + a3 > RENDER_WIDTH)
        v11 = RENDER_WIDTH - a1;
    if (a2 + a4 > RENDER_HEIGHT)
        v12 = RENDER_HEIGHT - a2;

    for (i = 0; i < v12; i++) {
        v13 = 0;
        v16 = 0;
        if (v11 > 0) {
            v14 = (double)(int)(int64_t)(((double)i + (double)a6) * 4.0f) * v15;
            do {
                int fi = (int)(int64_t)((double)(int)(int64_t)(((double)v16 + (double)a5) * 4.0f) + v14);
                if (fi >= 0 && fi < (int)sizeof(font_bitmap) && font_bitmap[fi]) {
                    if ((idx + v13) >= 0 && (idx + v13) < RENDER_WIDTH * RENDER_HEIGHT)
                        framebuffer[idx + v13] = a9;
                }
                v16 = ++v13;
            } while (v13 < v11);
        }
        idx += RENDER_WIDTH;
    }
}

/* ============================================================
 * sub_801BB0 - Render starfield (3D particle system)
 * ============================================================ */

static void render_starfield(void)
{
    /* sub_801BB0: direct port */
    int i;
    double drift_z, drift_x, drift_y;

    /* Update phase accumulators */
    phase2 += K_SPD6;
    phase6 += K_SPD5;
    phase3 += K_SPD4;
    phase4 += K_SPD3;
    phase5 += K_SPD2;
    phase1 += K_SPD1;

    /* Compute drift vectors from sine combinations */
    drift_x = (sin(phase1) + sin(phase2)) * K_AMP;
    drift_y = (sin(phase4) + sin(phase6)) * K_AMP;
    drift_z = (sin(phase5) + sin(phase3)) * K_AMP;

    /* Move stars and wrap around */
    for (i = 0; i < NUM_STARS; i++) {
        star_z[i] -= drift_z;
        if (star_z[i] < 0.0)
            star_z[i] += K_WRAP;
        else if (star_z[i] >= K_WRAP)
            star_z[i] -= K_WRAP;

        star_x[i] -= drift_x;
        if (star_x[i] < 0.0)
            star_x[i] += K_DEPTH;
        else if (star_x[i] >= K_DEPTH)
            star_x[i] -= K_DEPTH;

        star_y[i] -= drift_y;
        if (star_y[i] < 0.0)
            star_y[i] += K_DEPTH;
        else if (star_y[i] >= K_DEPTH)
            star_y[i] -= K_DEPTH;
    }

    /* Project and draw stars */
    for (i = 0; i < NUM_STARS; i++) {
        double z = star_z[i];
        int sx, sy, brightness;

        if (z == K_ZERO)
            z = K_POS1;

        /* Perspective projection */
        sx = 180 - (int)((star_x[i] - K_OFFSET) * (K_NEG1 / z) * K_PROJ);
        sy = 100 - (int)((star_y[i] - K_OFFSET) * (K_NEG1 / z) * K_PROJ);

        /* Distance-based brightness */
        brightness = 255 - (int)(z * K_FOGSCALE);

        /* Bounds check */
        if (sx < -16 || sx > 656 || sy < -16 || sy > 496)
            brightness = -1;

        if (brightness >= 0 && (unsigned)sx < RENDER_WIDTH && (unsigned)sy < RENDER_HEIGHT) {
            /* 65793 = 0x010101, so brightness * 0x010101 = gray */
            framebuffer[RENDER_WIDTH * sy + sx] = 65793 * brightness;
        }
    }
}

/* ============================================================
 * sub_801E50 - Render 3D animated logo
 * ============================================================ */

static void render_3d_logo(void)
{
    /* sub_801E50: direct port from IDA */
    int v1, v2, v3, v5;
    char* v4;

    if (LOGO_COLS <= 0)
        return;

    v1 = 0;
    v2 = frame_counter;
    v5 = 0;
    do {
        v3 = 0;
        v4 = (char*)&logo_pattern[v1];
        do {
            if (*v4 == '*') {
                render_textured_rect(
                    170 - ((int)(int64_t)(sin((double)(v5 + 24 * v2) * K_SINSCALE) * K_SINTAB) >> 5),
                    ((int)(int64_t)(sin((double)(32 * (v1 + 3 * v2)) * K_SINSCALE) * K_SINTAB) >> 8) + v3 + 48,
                    7,
                    7,
                    0,
                    0,
                    7);
            }
            v3 += 9;
            v4 += LOGO_COLS;
        } while (v3 < 72);
        ++v1;
        v5 += 64;
    } while (v1 < LOGO_COLS);
}

/* ============================================================
 * sub_801940 - Render scrolling text
 * ============================================================ */

static void render_scroll_text_line(void)
{
    /* sub_801940: direct port from IDA */
    int v0, v1, v2, v3, v5;
    int v8, v9, v10, v11, v12, v13, v14, v15, v16, v18, v19;
    int fc = frame_counter;

    v0 = scroll_char;
    v1 = scroll_x;

    if (v1 >= RENDER_WIDTH)
        goto done;

    v15 = 16 * (v1 + 8 * fc);
    v14 = 16 * (v1 + 6 * fc);
    v18 = 16 * (v1 + 7 * fc);
    v19 = 16 * (v1 + 10 * fc);
    v13 = 32 * (v1 + 3 * fc);

    do {
        v11 = v18;
        v12 = v13;
        if (v0 < scroll_display_len) {
            int ch = scroll_display_chars[v0];
            v16 = 8 * (ch / 16) - 16;
            v5 = 8 * (ch % 16) - v1;
        } else {
            v16 = 0;
            v5 = 0;
        }
        v10 = v19;
        v9 = v14;
        v8 = v15;

        for (int col = 0; col < 8; col++) {
            int draw_y = 96
                - ((int)(int64_t)(sin((double)v11 * K_SINSCALE) * K_SINTAB) >> 8)
                - ((int)(int64_t)(sin((double)v12 * K_SINSCALE) * K_SINTAB) >> 8);

            uint32_t color = 0x7F7F7F
                - ((((int)(int64_t)(sin((double)v9 * K_SINSCALE) * K_SINTAB) >> 6)
                       + ((int)(int64_t)(sin((double)v8 * K_SINSCALE) * K_SINTAB) >> 6 << 8))
                    << 8)
                - ((int)(int64_t)(sin((double)v10 * K_SINSCALE) * K_SINTAB) >> 6);

            render_masked_rect(
                v1 + col, draw_y, 1, 8,
                v5 + v1 + col, v16, 128, 48, color);

            v8 += 16;
            v9 += 16;
            v10 += 16;
            v11 += 16;
            v12 += 32;
        }

        v13 += 288;
        v1 += 9;
        v0++;
        v18 += 144;
        v19 += 144;
        v14 += 144;
        v15 += 144;
    } while (v1 < RENDER_WIDTH);

done:
    scroll_x--;
    if (scroll_x <= -16) {
        scroll_x = -8;
        scroll_char++;
    }
    if (scroll_char < scroll_display_len && scroll_display_chars[scroll_char] == '~') {
        scroll_char = 0;
        scroll_x = 0;
    }
}

/* ============================================================
 * sub_801F20 - Render thread init + frame
 * ============================================================ */

void render_init(void)
{
    int i;
    float v8 = 0.0f;

    memset(framebuffer, 0, sizeof(framebuffer));

    /* Random initial phases (original uses rand(), we use mt_rand) */
    phase2 = (double)(123 * (mt_rand() & 0xFFFF) % 255) * K_INV255;
    phase6 = (double)(123 * (mt_rand() & 0xFFFF) % 255) * K_INV255;
    phase3 = (double)(123 * (mt_rand() & 0xFFFF) % 255) * K_INV255;

    /* Initialize star positions */
    for (i = 0; i < NUM_STARS; i++) {
        star_x[i] = ((double)(123 * (mt_rand() & 0xFFFF) % 255) * K_INV255 - K_HALF) * K_DEPTH;
        star_y[i] = ((double)(123 * (mt_rand() & 0xFFFF) % 255) * K_INV255 - K_HALF) * K_DEPTH;
        star_z[i] = v8;
        v8 += 1.0f;
    }

    frame_counter = 0;
    scroll_x = 0;
    scroll_char = 0;
}

void render_frame(void)
{
    int i, v3;
    uint32_t gray = 0x00C0C0C0;

    /* Clear framebuffer (original: memset(dword_82F4F8, 0, 0x46500)) */
    memset(framebuffer, 0, sizeof(framebuffer));

    /* Render layers (same order as original sub_801F20) */
    render_starfield(); /* sub_801BB0 */
    render_3d_logo(); /* sub_801E50 */
    render_scroll_text_line(); /* sub_801940 */

    /* Bottom status bar: draw characters from status_charmap using font_bitmap */
    /* Original: for (i = 2; i < 353; i += 9) sub_801830(i, 190, 8, 8, ...) */
    v3 = 0;
    for (i = 2; i < 353 && v3 < status_charmap_len; i += 9) {
        uint8_t ch = status_charmap[v3];
        render_masked_rect(i, 190, 8, 8,
            8 * (ch % 16), 8 * (ch / 16) - 16,
            128, 48, 0x00C0C0C0);
        v3++;
    }

    /* Draw border lines: top row, bottom row = gray */
    for (i = 0; i < RENDER_WIDTH; i++) {
        framebuffer[i] = gray;
        framebuffer[(RENDER_HEIGHT - 1) * RENDER_WIDTH + i] = gray;
    }
    /* Left and right columns */
    for (i = 0; i < RENDER_HEIGHT; i++) {
        framebuffer[i * RENDER_WIDTH] = gray;
        framebuffer[i * RENDER_WIDTH + RENDER_WIDTH - 1] = gray;
    }

    frame_counter += 2;
}

uint32_t* render_get_framebuffer(void)
{
    return framebuffer;
}
