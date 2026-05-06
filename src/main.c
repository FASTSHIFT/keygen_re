/*
 * main.c - Demoscene demo (platform-independent core)
 *
 * A cross-platform demoscene application featuring FC14 Amiga music
 * playback, CPU software rasterization (starfield, 3D logo, scrolling
 * text), and a retro-style UI.
 *
 * All platform interaction goes through platform.h callbacks.
 * The concrete backend (SDL2) is selected at link time.
 *
 * UI layout from DLGTEMPLATEEX resource (dialog_keygen.bin):
 *   Dialog: 254x293 DLU, "MS Shell Dlg" 8pt
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "audio.h"
#include "font8x8.h"
#include "keygen.h"
#include "render.h"
#include "resources/resources.h"

/* Platform lifecycle (provided by platform_sdl2.c) */
extern void platform_sdl2_init(void);
extern void platform_sdl2_quit(void);

/* ============================================================
 * Window size derived from original dialog (254x293 DLU)
 * DLU->px at 96 DPI with MS Shell Dlg 8pt:
 *   horiz: 1 DLU = 1.5 px
 *   vert:  1 DLU = 1.8 px
 * ============================================================ */

#define DLU_H(x) ((x)*3 / 2)
#define DLU_V(y) ((y)*9 / 5)

#define WIN_WIDTH DLU_H(254) /* 381 */
#define WIN_HEIGHT DLU_V(293) /* 527 */

/* Colors matching original Win32 theme */
#define COL_BG 0xFF000000
#define COL_GRAY 0xFFC0C0C0
#define COL_WHITE 0xFFFFFFFF
#define COL_BLACK 0xFF000000
#define COL_DARKGRAY 0xFF404040
#define COL_EDIT_BG 0xFF000000

/* 3D animation area: original is at (10,40) in the window, 360x200 */
#define RENDER_X 10
#define RENDER_Y 35

/* ============================================================
 * UI state
 * ============================================================ */

static int target_index = 1; /* CB_SETCURSEL 1 -> "Developers Kit" */
static int license_index = 1; /* CB_SETCURSEL 1 -> "C251" */
static char cid_text[32] = "";
static char serial_text[64] = "";
static char output_text[64] = ""; /* edit_Output: serial or license key */
static int cid_cursor = 0;

/* ============================================================
 * Drawing primitives
 * ============================================================ */

static uint32_t screen_pixels[WIN_WIDTH * WIN_HEIGHT];

static void draw_char(int x, int y, char ch, uint32_t color)
{
    int row, col;
    int idx = (unsigned char)ch - 32;
    if (idx < 0 || idx >= 96)
        return;
    const uint8_t* glyph = font8x8_basic[idx];
    for (row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (col = 0; col < 8; col++) {
            if (bits & (1 << col)) {
                int px = x + col, py = y + row;
                if (px >= 0 && px < WIN_WIDTH && py >= 0 && py < WIN_HEIGHT)
                    screen_pixels[py * WIN_WIDTH + px] = color;
            }
        }
    }
}

static void draw_string(int x, int y, const char* str, uint32_t color)
{
    while (*str) {
        draw_char(x, y, *str, color);
        x += 8;
        str++;
    }
}

static void draw_rect(int x, int y, int w, int h, uint32_t color)
{
    int i, j;
    for (j = y; j < y + h && j < WIN_HEIGHT; j++)
        for (i = x; i < x + w && i < WIN_WIDTH; i++)
            if (i >= 0 && j >= 0)
                screen_pixels[j * WIN_WIDTH + i] = color;
}

static void draw_border(int x, int y, int w, int h, uint32_t color)
{
    int i;
    for (i = x; i < x + w; i++) {
        if (i >= 0 && i < WIN_WIDTH) {
            if (y >= 0 && y < WIN_HEIGHT)
                screen_pixels[y * WIN_WIDTH + i] = color;
            if (y + h - 1 >= 0 && y + h - 1 < WIN_HEIGHT)
                screen_pixels[(y + h - 1) * WIN_WIDTH + i] = color;
        }
    }
    for (i = y; i < y + h; i++) {
        if (i >= 0 && i < WIN_HEIGHT) {
            if (x >= 0 && x < WIN_WIDTH)
                screen_pixels[i * WIN_WIDTH + x] = color;
            if (x + w - 1 >= 0 && x + w - 1 < WIN_WIDTH)
                screen_pixels[i * WIN_WIDTH + x + w - 1] = color;
        }
    }
}

/* Draw a Win32-style group box with label */
static void draw_groupbox(int x, int y, int w, int h,
    const char* label, uint32_t color)
{
    int lx = x + 8;
    int lw = (int)strlen(label) * 8 + 4;
    /* Top line with gap for label */
    int i;
    for (i = x; i < x + w; i++) {
        if (i >= lx - 2 && i < lx + lw)
            continue; /* gap */
        if (i >= 0 && i < WIN_WIDTH && y >= 0 && y < WIN_HEIGHT)
            screen_pixels[y * WIN_WIDTH + i] = color;
    }
    /* Bottom, left, right */
    for (i = x; i < x + w; i++)
        if (i >= 0 && i < WIN_WIDTH && y + h - 1 >= 0 && y + h - 1 < WIN_HEIGHT)
            screen_pixels[(y + h - 1) * WIN_WIDTH + i] = color;
    for (i = y; i < y + h; i++) {
        if (i >= 0 && i < WIN_HEIGHT) {
            if (x >= 0 && x < WIN_WIDTH)
                screen_pixels[i * WIN_WIDTH + x] = color;
            if (x + w - 1 >= 0 && x + w - 1 < WIN_WIDTH)
                screen_pixels[i * WIN_WIDTH + x + w - 1] = color;
        }
    }
    draw_string(lx, y - 4, label, color);
}

/* ============================================================
 * Button definitions (pixel coords derived from DLU)
 * ============================================================ */

typedef struct {
    int x, y, w, h;
    const char* label;
} Button;

static Button btn_gen_serial = { 0 };
static Button btn_open_request = { 0 };
static Button btn_target_l = { 0 };
static Button btn_target_r = { 0 };
static Button btn_license_l = { 0 };
static Button btn_license_r = { 0 };

static void init_buttons(void)
{
    btn_gen_serial = (Button) { DLU_H(45), DLU_V(265), DLU_H(70), DLU_V(14), "Gen. Serial" };
    btn_open_request = (Button) { DLU_H(120), DLU_V(265), DLU_H(75), DLU_V(14), "Open Request" };
    btn_target_l = (Button) { DLU_H(216), DLU_V(210), DLU_H(7), DLU_V(12), "<" };
    btn_target_r = (Button) { DLU_H(223), DLU_V(210), DLU_H(7), DLU_V(12), ">" };
    btn_license_l = (Button) { DLU_H(217), DLU_V(182), DLU_H(7), DLU_V(12), "<" };
    btn_license_r = (Button) { DLU_H(224), DLU_V(182), DLU_H(7), DLU_V(12), ">" };
}

static int point_in(Button* b, int mx, int my)
{
    return mx >= b->x && mx < b->x + b->w && my >= b->y && my < b->y + b->h;
}

/* ============================================================
 * Event handling
 * ============================================================ */

static int handle_events(void)
{
    PlatformEvent ev;
    while (g_input.poll(&ev)) {
        switch (ev.type) {
        case EVT_QUIT:
            return 0;

        case EVT_MOUSE_DOWN: {
            int mx = ev.mouse.x, my = ev.mouse.y;
            if (point_in(&btn_gen_serial, mx, my)) {
                generate_serial(output_text, 0, target_index, license_index);
                printf("Serial: %s\n", output_text);
            } else if (point_in(&btn_open_request, mx, my)) {
                if (strlen(cid_text) == 11) {
                    generate_license_key(output_text, cid_text, 1,
                        target_index, license_index);
                    printf("License: %s\n", output_text);
                } else {
                    snprintf(output_text, sizeof(output_text),
                        "Invalid Computer ID!!");
                }
            } else if (point_in(&btn_target_r, mx, my)) {
                target_index = (target_index + 1) % 11;
            } else if (point_in(&btn_target_l, mx, my)) {
                target_index = (target_index + 10) % 11;
            } else if (point_in(&btn_license_r, mx, my)) {
                license_index = (license_index + 1) % 4;
            } else if (point_in(&btn_license_l, mx, my)) {
                license_index = (license_index + 3) % 4;
            }
            break;
        }

        case EVT_KEY_DOWN:
            if (ev.key.sym == PKEY_ESCAPE)
                return 0;
            else if (ev.key.sym == PKEY_BACKSPACE) {
                if (cid_cursor > 0)
                    cid_text[--cid_cursor] = '\0';
            }
            break;

        case EVT_TEXT_INPUT: {
            char ch = ev.text.text[0];
            if (cid_cursor < 11 && ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '-')) {
                if (ch >= 'a' && ch <= 'z')
                    ch -= 32;
                cid_text[cid_cursor++] = ch;
                cid_text[cid_cursor] = '\0';
            }
            break;
        }

        default:
            break;
        }
    }
    return 1; /* keep running */
}

/* ============================================================
 * UI composition
 * ============================================================ */

static void compose_ui(void)
{
    memset(screen_pixels, 0, sizeof(screen_pixels));

    /* Title bar */
    draw_border(5, 5, WIN_WIDTH - 10, 25, COL_GRAY);
    draw_string(28, 10, "Keil Generic Keygen - EDGE", COL_GRAY);

    /* 3D animation area */
    {
        uint32_t* fb = render_get_framebuffer();
        int y, x;
        for (y = 0; y < RENDER_HEIGHT; y++)
            for (x = 0; x < RENDER_WIDTH; x++) {
                int sx = RENDER_X + x, sy = RENDER_Y + y;
                if (sx < WIN_WIDTH && sy < WIN_HEIGHT)
                    screen_pixels[sy * WIN_WIDTH + sx] = fb[y * RENDER_WIDTH + x] | 0xFF000000;
            }
    }

    /* "Keygen" group box - DLU (7,152) 239x136 */
    draw_groupbox(DLU_H(7), DLU_V(152), DLU_H(239), DLU_V(136), "Keygen", COL_GRAY);

    /* "License Details" group box - DLU (20,168) 211x64 */
    draw_groupbox(DLU_H(20), DLU_V(168), DLU_H(211), DLU_V(64), "License Details", COL_GRAY);

    /* CID label + input */
    draw_string(DLU_H(26), DLU_V(184), "CID:", COL_GRAY);
    draw_border(DLU_H(42), DLU_V(182), DLU_H(63), DLU_V(13), COL_GRAY);
    draw_rect(DLU_H(42) + 1, DLU_V(182) + 1, DLU_H(63) - 2, DLU_V(13) - 2, COL_EDIT_BG);
    draw_string(DLU_H(42) + 3, DLU_V(182) + 5, cid_text, COL_WHITE);

    /* "Target" label + license combo */
    draw_string(DLU_H(120), DLU_V(185), "Target", COL_GRAY);
    draw_border(DLU_H(145), DLU_V(182), DLU_H(72), DLU_V(13), COL_GRAY);
    draw_rect(DLU_H(145) + 1, DLU_V(182) + 1, DLU_H(72) - 2, DLU_V(13) - 2, COL_EDIT_BG);
    draw_string(DLU_H(145) + 3, DLU_V(182) + 5, license_names[license_index], COL_WHITE);
    /* License arrows */
    draw_rect(btn_license_l.x, btn_license_l.y, btn_license_l.w, btn_license_l.h, COL_DARKGRAY);
    draw_string(btn_license_l.x + 2, btn_license_l.y + 6, "<", COL_WHITE);
    draw_rect(btn_license_r.x, btn_license_r.y, btn_license_r.w, btn_license_r.h, COL_DARKGRAY);
    draw_string(btn_license_r.x + 2, btn_license_r.y + 6, ">", COL_WHITE);

    /* Product combo (target type) - DLU (41,210) 175x13 */
    draw_border(DLU_H(41), DLU_V(210), DLU_H(175), DLU_V(13), COL_GRAY);
    draw_rect(DLU_H(41) + 1, DLU_V(210) + 1, DLU_H(175) - 2, DLU_V(13) - 2, COL_EDIT_BG);
    draw_string(DLU_H(41) + 3, DLU_V(210) + 5, target_names[target_index], COL_WHITE);
    /* Target arrows */
    draw_rect(btn_target_l.x, btn_target_l.y, btn_target_l.w, btn_target_l.h, COL_DARKGRAY);
    draw_string(btn_target_l.x + 2, btn_target_l.y + 6, "<", COL_WHITE);
    draw_rect(btn_target_r.x, btn_target_r.y, btn_target_r.w, btn_target_r.h, COL_DARKGRAY);
    draw_string(btn_target_r.x + 2, btn_target_r.y + 6, ">", COL_WHITE);

    /* Output edit box - DLU (20,240) 211x15 */
    draw_border(DLU_H(20), DLU_V(240), DLU_H(211), DLU_V(15), COL_GRAY);
    draw_rect(DLU_H(20) + 1, DLU_V(240) + 1, DLU_H(211) - 2, DLU_V(15) - 2, COL_EDIT_BG);
    draw_string(DLU_H(20) + 3, DLU_V(240) + 7, output_text, COL_WHITE);

    /* Gen. Serial button */
    draw_border(btn_gen_serial.x, btn_gen_serial.y,
        btn_gen_serial.w, btn_gen_serial.h, COL_GRAY);
    draw_rect(btn_gen_serial.x + 1, btn_gen_serial.y + 1,
        btn_gen_serial.w - 2, btn_gen_serial.h - 2, COL_DARKGRAY);
    {
        int tw = (int)strlen(btn_gen_serial.label) * 8;
        draw_string(btn_gen_serial.x + (btn_gen_serial.w - tw) / 2,
            btn_gen_serial.y + 9, btn_gen_serial.label, COL_WHITE);
    }

    /* Open Request button */
    draw_border(btn_open_request.x, btn_open_request.y,
        btn_open_request.w, btn_open_request.h, COL_GRAY);
    draw_rect(btn_open_request.x + 1, btn_open_request.y + 1,
        btn_open_request.w - 2, btn_open_request.h - 2, COL_DARKGRAY);
    {
        int tw = (int)strlen(btn_open_request.label) * 8;
        draw_string(btn_open_request.x + (btn_open_request.w - tw) / 2,
            btn_open_request.y + 9, btn_open_request.label, COL_WHITE);
    }
}

/* ============================================================
 * Main
 * ============================================================ */

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    platform_sdl2_init();

    if (g_video.open("Keil Generic Keygen - EDGE", WIN_WIDTH, WIN_HEIGHT) < 0) {
        fprintf(stderr, "Failed to open video\n");
        platform_sdl2_quit();
        return 1;
    }

    mt_seed((uint32_t)time(NULL));
    init_buttons();
    render_init();
    audio_init(tune_data, sizeof(tune_data));

    printf("Keil Generic Keygen - EDGE (Linux SDL2 port)\n");

    while (handle_events()) {
        render_frame();
        compose_ui();
        g_video.present(screen_pixels, WIN_WIDTH, WIN_HEIGHT);
        g_input.delay_ms(25);
    }

    audio_shutdown();
    g_video.close();
    platform_sdl2_quit();
    return 0;
}
