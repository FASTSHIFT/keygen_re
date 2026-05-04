/*
 * render.h - 3D rendering subsystem (CPU software rasterizer)
 * Ported from IDA decompilation: sub_801F20, sub_801BB0, sub_801940, sub_801E50, sub_801830, sub_8016C0
 */
#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

#define RENDER_WIDTH 360
#define RENDER_HEIGHT 200

/* Initialize rendering state (random star positions, etc.) */
void render_init(void);

/* Render one frame into the framebuffer */
void render_frame(void);

/* Get pointer to the 360x200 ARGB framebuffer */
uint32_t* render_get_framebuffer(void);

#endif /* RENDER_H */
