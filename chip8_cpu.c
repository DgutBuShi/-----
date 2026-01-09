#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "chip8_cpu.h"
#include "chip8_opcodes.h"

// 全局变量定义
chip8_cpu_t* CHIP8_CPU = NULL;
float speed_coeff = 1.0f;        // 速度系数（默认100%）
int is_running = 1;              // 程序运行标记

// CHIP-8内置字体集（0-F点阵）
static const unsigned char FONTSET[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// 初始化CPU（首次启动，分配内存+加载字体）
void init(void)
{
    // 分配CPU内存
    CHIP8_CPU = (chip8_cpu_t*)malloc(sizeof(chip8_cpu_t));
    if (!CHIP8_CPU) {
        fprintf(stderr, "Failed to allocate CPU memory\n");
        exit(EXIT_FAILURE);
    }

    // 重置CPU状态（复用reset逻辑）
    reset();

    // 加载字体集到内存（仅首次初始化执行）
    memcpy(CHIP8_CPU->memory + FONTSET_START_ADDR, FONTSET, sizeof(FONTSET));

    // 初始化随机数种子
    srand(time(NULL));
}

// 重置CPU（加载新ROM时调用，保留内存/字体，重置寄存器/定时器等）
void reset(void)
{
    if (!CHIP8_CPU) return;

    // 重置核心硬件状态
    memset(CHIP8_CPU->registers, 0, sizeof(CHIP8_CPU->registers));
    memset(CHIP8_CPU->stack, 0, sizeof(CHIP8_CPU->stack));
    memset(CHIP8_CPU->video, 0, sizeof(CHIP8_CPU->video));
    memset(CHIP8_CPU->keypad, 0, sizeof(CHIP8_CPU->keypad));

    CHIP8_CPU->index = 0;
    CHIP8_CPU->pc = PROGRAM_START_ADDR;  // 程序计数器指向ROM起始地址
    CHIP8_CPU->sp = 0;
    CHIP8_CPU->delayTimer = 0;
    CHIP8_CPU->soundTimer = 0;
    CHIP8_CPU->opcode = 0;
    CHIP8_CPU->draw_flag = 1; // 重置后清屏
}

// 释放CPU内存
void destroy(void)
{
    if (CHIP8_CPU) {
        free(CHIP8_CPU);
        CHIP8_CPU = NULL;
    }
}

// 加载ROM文件到内存（0x200开始）
int loadrom(const char* rom_path)
{
    if (!rom_path || !CHIP8_CPU) return -1;

    FILE* rom_file = fopen(rom_path, "rb");
    if (!rom_file) {
        fprintf(stderr, "Failed to open ROM file: %s\n", rom_path);
        return -1;
    }

    // 获取ROM大小
    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);

    // 检查ROM大小是否超出内存限制（0x200-0xFFF，共3840字节）
    if (rom_size > (4096 - PROGRAM_START_ADDR)) {
        fprintf(stderr, "ROM file too large (max size: %d bytes)\n", 4096 - PROGRAM_START_ADDR);
        fclose(rom_file);
        return -1;
    }

    // 读取ROM到内存（0x200开始）
    size_t bytes_read = fread(CHIP8_CPU->memory + PROGRAM_START_ADDR, 1, rom_size, rom_file);
    fclose(rom_file);

    if (bytes_read != rom_size) {
        fprintf(stderr, "Failed to read full ROM (read %zu/%ld bytes)\n", bytes_read, rom_size);
        return -1;
    }

    printf("Successfully loaded ROM: %s (size: %ld bytes)\n", rom_path, rom_size);
    return 0;
}

// 定时器更新计数器（适配速度系数）
static uint32_t timer_ticks = 0;
#define BASE_TIMER_FREQ 60 // 基准定时器频率60Hz

// 执行一次CPU周期（取指→解码→执行→更新定时器）
void cycle(void)
{
    // 1. 取指：从PC读取16位指令（CHIP-8指令为2字节）
    CHIP8_CPU->opcode = (CHIP8_CPU->memory[CHIP8_CPU->pc] << 8) | CHIP8_CPU->memory[CHIP8_CPU->pc + 1];

    // 2. 步进PC（指向下一条指令，指令执行时可能修改）
    CHIP8_CPU->pc += 2;

    // 3. 解码并执行指令
    oc_exec();

    // 4. 更新定时器（按速度系数适配频率）
    timer_ticks++;
    // 计算适配速度后的定时器更新阈值（BASE_TIMER_FREQ * speed_coeff）
    uint32_t timer_threshold = (uint32_t)(BASE_CYCLES_PER_FRAME * 60 / (BASE_TIMER_FREQ * speed_coeff));
    if (timer_ticks >= timer_threshold) {
        if (CHIP8_CPU->delayTimer > 0) {
            CHIP8_CPU->delayTimer--;
        }

        if (CHIP8_CPU->soundTimer > 0) {
            CHIP8_CPU->soundTimer--;
            // 声音定时器>0时可触发蜂鸣（简化实现，注释掉避免依赖音频）
            // audio_beep();
        }
        timer_ticks = 0;
    }
}
