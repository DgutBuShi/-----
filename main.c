#define _CRT_SECURE_NO_WARNINGS
#ifdef _WIN32
#pragma comment(lib, "shell32.lib")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "chip8_cpu.h"
#include "chip8_platform.h"

#define FPS 60
#define FRAME_DELAY (1000 / FPS)

int main(int argc, char* argv[])
{
    // 初始化CPU
    init();

    // 若命令行传入ROM路径，直接加载
    if (argc >= 2) {
        if (loadrom(argv[1]) != 0) {
            destroy();
            return EXIT_FAILURE;
        }
    }
    else {
        printf("No ROM path provided - drag .ch8 file to the window to load\n");
    }

    // 初始化SDL平台（显示/音频/字体）
    display_init();

    // 主循环
    uint32_t frame_start;
    int frame_time;

    while (is_running)
    {
        frame_start = SDL_GetTicks();

        // 1. 检测输入（键盘/拖放/窗口关闭）
        input_detect();

        // 2. 执行CPU周期（按速度系数调整每帧执行次数）
        int cycles_per_frame = (int)(BASE_CYCLES_PER_FRAME * speed_coeff);
        for (int i = 0; i < cycles_per_frame; i++) {
            cycle();
        }

        // 3. 刷新屏幕（如果需要）
        if (CHIP8_CPU->draw_flag) {
            display_update();
            CHIP8_CPU->draw_flag = 0;
        }

        // 4. 控制帧率（固定60Hz）
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    // 清理资源
    display_destroy();
    audio_destroy();
    destroy();

    return EXIT_SUCCESS;
}
