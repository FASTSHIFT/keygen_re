# SDL2 解耦方案：基于回调的平台抽象层

## 1. 现状分析

### 当前架构

```
main.c (SDL事件循环 + UI绘制 + SDL渲染输出)
├── audio.c (c-flod播放器 + SDL音频设备)
├── render.c (纯CPU软光栅化，无SDL依赖 ✓)
└── keygen.c (纯算法，无SDL依赖 ✓)
```

### SDL 耦合点

| 模块 | 耦合的 SDL API | 耦合程度 |
|------|---------------|---------|
| audio.c | SDL_OpenAudioDevice, SDL_PauseAudioDevice, SDL_CloseAudioDevice | 强 |
| main.c | SDL_Init, SDL_CreateWindow, SDL_CreateRenderer, SDL_CreateTexture | 强 |
| main.c | SDL_PollEvent, SDL_Delay | 强 |
| main.c | SDL_UpdateTexture, SDL_RenderCopy, SDL_RenderPresent | 强 |
| render.c | 无 | 无 |
| keygen.c | 无 | 无 |

### 已有的良好设计

- `render.c` 已经完全解耦：输出到 `uint32_t framebuffer[360×200]`，通过 `render_get_framebuffer()` 暴露
- `keygen.c` 纯逻辑，零平台依赖
- 音频回调 `audio_callback()` 内部逻辑（c-flod mixer → memcpy）本身与 SDL 无关，只是签名绑定了 SDL

## 2. 目标

1. 核心逻辑（音频生成、渲染、UI状态机）不再 `#include <SDL.h>`
2. SDL 作为可替换的后端之一，集中在单独文件中
3. 未来可轻松替换为其他后端（GLFW+OpenAL、Win32原生、headless测试等）
4. 改动量最小，不重构 render.c / keygen.c（它们已经干净）

## 3. 设计方案

### 3.1 平台抽象层 (`src/platform.h`)

```c
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

/*
 * 平台后端通过填充这些回调表来注册自己。
 * 核心逻辑只通过这些接口与外界交互。
 */

/* --- 音频后端 --- */
typedef void (*audio_fill_fn)(void* userdata, uint8_t* stream, int len);

typedef struct {
    int  (*open)(int freq, int channels, int samples,
                 audio_fill_fn fill_cb, void* userdata);
    void (*close)(void);
    void (*pause)(int pause_on);
} PlatformAudio;

/* --- 视频后端 --- */
typedef struct {
    int  (*open)(const char* title, int width, int height);
    void (*present)(const uint32_t* pixels, int width, int height);
    void (*close)(void);
} PlatformVideo;

/* --- 输入事件 --- */
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

/* 键码常量（与 SDL 解耦） */
#define PKEY_ESCAPE    0x1B
#define PKEY_BACKSPACE 0x08

typedef struct {
    int  (*poll)(PlatformEvent* event);  /* 返回1有事件，0无 */
    void (*delay_ms)(int ms);
} PlatformInput;

/* --- 全局后端实例（由 main 在启动时赋值） --- */
extern PlatformAudio  g_audio;
extern PlatformVideo  g_video;
extern PlatformInput  g_input;

#endif /* PLATFORM_H */
```

### 3.2 SDL2 后端实现 (`src/platform_sdl2.c`)

集中所有 `#include <SDL.h>` 到这一个文件：

```c
#include "platform.h"
#include <SDL.h>

/* --- Audio --- */
static SDL_AudioDeviceID s_dev;

static int sdl_audio_open(int freq, int channels, int samples,
                          audio_fill_fn fill_cb, void* userdata) {
    SDL_AudioSpec want = {0};
    want.freq = freq;
    want.format = AUDIO_S16SYS;
    want.channels = channels;
    want.samples = samples;
    want.callback = (SDL_AudioCallback)fill_cb;
    want.userdata = userdata;
    s_dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    return s_dev ? 0 : -1;
}
static void sdl_audio_close(void) { SDL_CloseAudioDevice(s_dev); }
static void sdl_audio_pause(int p) { SDL_PauseAudioDevice(s_dev, p); }

/* --- Video --- */
static SDL_Window*   s_win;
static SDL_Renderer* s_ren;
static SDL_Texture*  s_tex;
static int s_w, s_h;

static int sdl_video_open(const char* title, int w, int h) {
    s_w = w; s_h = h;
    s_win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    if (!s_win) return -1;
    s_ren = SDL_CreateRenderer(s_win, -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_ren) s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_SOFTWARE);
    s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING, w, h);
    return 0;
}
static void sdl_video_present(const uint32_t* px, int w, int h) {
    SDL_UpdateTexture(s_tex, NULL, px, w * 4);
    SDL_RenderClear(s_ren);
    SDL_RenderCopy(s_ren, s_tex, NULL, NULL);
    SDL_RenderPresent(s_ren);
}
static void sdl_video_close(void) {
    SDL_DestroyTexture(s_tex);
    SDL_DestroyRenderer(s_ren);
    SDL_DestroyWindow(s_win);
}

/* --- Input --- */
static int sdl_poll(PlatformEvent* ev) {
    SDL_Event e;
    if (!SDL_PollEvent(&e)) return 0;
    ev->type = EVT_NONE;
    switch (e.type) {
    case SDL_QUIT: ev->type = EVT_QUIT; break;
    case SDL_MOUSEBUTTONDOWN:
        ev->type = EVT_MOUSE_DOWN;
        ev->mouse.x = e.button.x;
        ev->mouse.y = e.button.y;
        ev->mouse.button = e.button.button;
        break;
    case SDL_KEYDOWN:
        ev->type = EVT_KEY_DOWN;
        ev->key.sym = (e.key.keysym.sym == SDLK_ESCAPE) ? PKEY_ESCAPE :
                      (e.key.keysym.sym == SDLK_BACKSPACE) ? PKEY_BACKSPACE :
                      e.key.keysym.sym;
        break;
    case SDL_TEXTINPUT:
        ev->type = EVT_TEXT_INPUT;
        memcpy(ev->text.text, e.text.text, sizeof(ev->text.text));
        break;
    }
    return 1;
}
static void sdl_delay(int ms) { SDL_Delay(ms); }

/* --- 注册 --- */
PlatformAudio g_audio = { sdl_audio_open, sdl_audio_close, sdl_audio_pause };
PlatformVideo g_video = { sdl_video_open, sdl_video_present, sdl_video_close };
PlatformInput g_input = { sdl_poll, sdl_delay };

void platform_sdl2_init(void) { SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); }
void platform_sdl2_quit(void) { SDL_Quit(); }
```

### 3.3 改造后的 audio.c

```c
#include "audio.h"
#include "platform.h"    /* 不再 #include <SDL.h> */
/* ... c-flod headers ... */

static void audio_fill(void* userdata, uint8_t* stream, int len) {
    /* 与现在的 audio_callback 逻辑完全相同 */
}

int audio_init(const uint8_t* tune_data, int tune_size) {
    /* ... c-flod 初始化不变 ... */
    if (g_audio.open(44100, 2, 2048, audio_fill, NULL) < 0) return -1;
    g_audio.pause(0);
    return 0;
}

void audio_shutdown(void) {
    g_audio.pause(1);
    g_audio.close();
}
```

### 3.4 改造后的 main.c

```c
#include "platform.h"    /* 不再 #include <SDL.h> */
#include "audio.h"
#include "render.h"
#include "keygen.h"

extern void platform_sdl2_init(void);
extern void platform_sdl2_quit(void);

int main(int argc, char* argv[]) {
    platform_sdl2_init();
    g_video.open("Keil Generic Keygen - EDGE", WIN_WIDTH, WIN_HEIGHT);
    /* ... init_buttons, render_init, audio_init 不变 ... */

    while (running) {
        PlatformEvent ev;
        while (g_input.poll(&ev)) {
            switch (ev.type) {
            case EVT_QUIT: running = 0; break;
            case EVT_MOUSE_DOWN: /* 同现有逻辑 */ break;
            case EVT_KEY_DOWN:   /* 同现有逻辑 */ break;
            case EVT_TEXT_INPUT: /* 同现有逻辑 */ break;
            }
        }
        render_frame();
        compose_ui(screen_pixels);       /* UI绘制提取为函数 */
        g_video.present(screen_pixels, WIN_WIDTH, WIN_HEIGHT);
        g_input.delay_ms(25);
    }

    audio_shutdown();
    g_video.close();
    platform_sdl2_quit();
}
```

## 4. 文件变更清单

| 操作 | 文件 | 说明 |
|------|------|------|
| 新增 | `src/platform.h` | 平台抽象接口定义 |
| 新增 | `src/platform_sdl2.c` | SDL2 后端实现（唯一包含 SDL.h 的文件） |
| 改造 | `src/audio.c` | 去掉 `#include <SDL.h>`，改用 `g_audio.*` |
| 改造 | `src/main.c` | 去掉 `#include <SDL.h>`，改用 `g_video.*` / `g_input.*` |
| 不动 | `src/render.c` | 已解耦 |
| 不动 | `src/keygen.c` | 已解耦 |
| 改造 | `CMakeLists.txt` | 添加 `platform_sdl2.c`；SDL2 include/link 只给该文件 |

## 5. CMake 改造思路

```cmake
# 核心库：不链接 SDL
add_library(app_core STATIC
    src/audio.c
    src/render.c
    src/keygen.c
)
target_include_directories(app_core PRIVATE src)

# SDL2 后端
add_library(platform_sdl2 STATIC src/platform_sdl2.c)
target_include_directories(platform_sdl2 PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(platform_sdl2 PRIVATE ${SDL2_LIBRARIES})

# 可执行文件
add_executable(keygen_re src/main.c)
target_link_libraries(keygen_re app_core platform_sdl2 cflod m)
```

这样 `app_core` 的编译完全不需要 SDL 头文件，保证了隔离。

## 6. 后续扩展

- 添加 `platform_headless.c`：用于 CI 自动化测试（音频静默、视频写 PNG）
- 添加 `platform_win32.c`：原生 Win32 API 后端（还原原始 exe 的行为）
- 添加 `platform_glfw.c`：GLFW + OpenAL 后端

## 7. 实施步骤

1. 新建 `src/platform.h`，定义接口
2. 新建 `src/platform_sdl2.c`，把 SDL 调用搬过去
3. 改造 `src/audio.c`：替换 SDL 调用为 `g_audio.*`
4. 改造 `src/main.c`：替换 SDL 调用为 `g_video.*` / `g_input.*`
5. 更新 `CMakeLists.txt`
6. 验证构建 + 运行
