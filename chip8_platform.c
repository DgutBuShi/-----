#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8_platform.h"
#include "chip8_cpu.h"

// 全局SDL资源
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
SDL_AudioDeviceID audio_device;

// 键盘映射（CHIP-8 0-F → PC键盘）
static const int key_map[16] = {
    SDLK_x,    // 0
    SDLK_1,    // 1
    SDLK_2,    // 2
    SDLK_3,    // 3
    SDLK_q,    // 4
    SDLK_w,    // 5
    SDLK_e,    // 6
    SDLK_r,    // 7
    SDLK_a,    // 8
    SDLK_s,    // 9
    SDLK_d,    // A
    SDLK_z,    // B
    SDLK_c,    // C
    SDLK_4,    // D
    SDLK_f,    // E
    SDLK_v     // F
};

// 音频回调函数（生成方波蜂鸣）
static void audio_callback(void* userdata, Uint8* stream, int len)
{
    static int sample_pos = 0;
    int amplitude = 2000; // 音量
    int freq = 440;       // 蜂鸣频率（440Hz）
    int sample_rate = 44100;

    memset(stream, 0, len);
    if (CHIP8_CPU->soundTimer == 0) return;

    // 生成方波
    for (int i = 0; i < len; i += 2) {
        int16_t sample = (sample_pos < (sample_rate / freq / 2)) ? amplitude : -amplitude;
        ((int16_t*)stream)[i / 2] = sample;
        sample_pos = (sample_pos + 1) % (sample_rate / freq);
    }
}

// 初始化SDL显示+字体+音频
void display_init(void)
{
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // 初始化SDL_ttf（用于显示速度百分比）
    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf init failed: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    // 创建窗口
    window = SDL_CreateWindow(
        "CHIP-8 Emulator (Drag .ch8 ROM to load | +/- adjust speed)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "Window create failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // 创建渲染器
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "Renderer create failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // 加载字体（使用系统默认字体，可替换为实际字体路径）
    font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 16); // Windows示例
    // font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 16); // Linux示例
    if (!font) {
        fprintf(stderr, "Font load failed: %s (use default font)\n", TTF_GetError());
        font = TTF_OpenFont(TTF_GetDefaultFont(), 16);
    }

    // 初始化音频
    audio_init();
}

// 更新屏幕显示（绘制像素+速度百分比）
void display_update(void)
{
    if (!renderer || !CHIP8_CPU) return;

    // 清屏（黑色背景）
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // 绘制CHIP-8像素（白色）
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (CHIP8_CPU->video[y * SCREEN_WIDTH + x]) {
                SDL_Rect rect = {
                    x * SCALE,
                    y * SCALE,
                    SCALE,
                    SCALE
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    // 绘制速度百分比文本（左上角，红色）
    char speed_text[32];
    snprintf(speed_text, sizeof(speed_text), "Speed: %.0f%%", speed_coeff * 100);
    SDL_Color text_color = { 255, 0, 0, 255 }; // 红色
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, speed_text, text_color);
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);

    if (text_texture) {
        SDL_Rect text_rect = { 5, 5, text_surface->w, text_surface->h };
        SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
        SDL_DestroyTexture(text_texture);
    }
    SDL_FreeSurface(text_surface);

    // 刷新屏幕
    SDL_RenderPresent(renderer);
}

// 释放SDL资源
void display_destroy(void)
{
    // 释放字体
    if (font) {
        TTF_CloseFont(font);
        font = NULL;
    }

    // 释放渲染器/窗口
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    // 退出SDL_ttf/SDL
    TTF_Quit();
    SDL_Quit();
}

// 检测键盘输入（含速度调节、CHIP-8按键、窗口关闭）
void input_detect(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // 窗口关闭事件
        if (event.type == SDL_QUIT) {
            is_running = 0;
            return;
        }

        // ROM拖放事件
        if (event.type == SDL_DROPFILE) {
            char* file_path = event.drop.file;
            // 校验文件后缀是否为.ch8
            char* ext = strrchr(file_path, '.');
            if (ext && strcmp(ext, ".ch8") == 0) {
                // 重置CPU + 加载新ROM
                reset();
                if (loadrom(file_path) == 0) {
                    printf("Loaded ROM via drag&drop: %s\n", file_path);
                }
                else {
                    fprintf(stderr, "Failed to load ROM: %s\n", file_path);
                }
            }
            else {
                fprintf(stderr, "Unsupported file format (only .ch8 allowed): %s\n", file_path);
            }
            SDL_free(file_path); // 释放SDL分配的路径内存
            continue;
        }

        // 键盘按下/释放事件
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            // 速度调节：+键（主键盘/小键盘）增加速度（上限200%）
            if (event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS) {
                if (event.type == SDL_KEYDOWN) {
                    speed_coeff = (speed_coeff + 0.1f > 2.0f) ? 2.0f : speed_coeff + 0.1f;
                    printf("Speed adjusted to: %.0f%%\n", speed_coeff * 100);
                }
                continue;
            }
            // 速度调节：-键（主键盘/小键盘）降低速度（下限50%）
            if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS) {
                if (event.type == SDL_KEYDOWN) {
                    speed_coeff = (speed_coeff - 0.1f < 0.5f) ? 0.5f : speed_coeff - 0.1f;
                    printf("Speed adjusted to: %.0f%%\n", speed_coeff * 100);
                }
                continue;
            }
            // ESC键退出程序
            if (event.key.keysym.sym == SDLK_ESCAPE && event.type == SDL_KEYDOWN) {
                is_running = 0;
                return;
            }

            // CHIP-8按键映射
            for (int i = 0; i < 16; i++) {
                if (event.key.keysym.sym == key_map[i]) {
                    CHIP8_CPU->keypad[i] = (event.type == SDL_KEYDOWN) ? 1 : 0;
                }
            }
        }
    }
}

// 初始化音频（简化实现）
void audio_init(void)
{
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (audio_device == 0) {
        fprintf(stderr, "Audio init failed: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audio_device, 0); // 启动音频播放
}

// 释放音频资源
void audio_destroy(void)
{
    SDL_CloseAudioDevice(audio_device);
}

// 蜂鸣音效（简化实现，实际由音频回调处理）
void audio_beep(void)
{
    // 空实现（音频回调已处理）
}
