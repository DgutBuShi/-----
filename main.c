#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <stdlib.h> 
#pragma comment(lib, "shell32.lib")

#include "chip8_cpu.h"
#include "chip8_platform.h"

#define FPS 60
#define FRAME_DELAY (1000 / FPS)

int main(int argc, char* argv[])
{
    // 检查命令行参数
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ROM_FILE_PATH>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 初始化CPU
    init();

    // 加载ROM
    if (loadrom(argv[1]) != 0) {
        destroy();
        return EXIT_FAILURE;
    }

    // 初始化平台（显示+音频）
    display_init();
    audio_init();

    // 设置运行标志
    is_running = 1;

    // 主循环
    uint32_t frame_start;
    int frame_time;

    while (is_running)
    {
        frame_start = SDL_GetTicks();

        // 1. 检测输入
        input_detect();

        // 2. 执行CPU周期（每帧执行9个cycle，对应60Hz）
        for (int i = 0; i < CYCLES_PER_FRAME; i++) {
            cycle();
        }

        // 3. 刷新屏幕（如果需要）
        if (CHIP8_CPU->draw_flag) {
            display_update();
            CHIP8_CPU->draw_flag = 0;
        }

        // 4. 控制帧率
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
