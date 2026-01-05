#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "chip8_cpu.h"
#include "chip8_opcodes.h"

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

chip8_cpu_t* CHIP8_CPU = NULL;

void init(void)
{
    // 分配CPU内存
    CHIP8_CPU = (chip8_cpu_t*)malloc(sizeof(chip8_cpu_t));
    if (!CHIP8_CPU) {
        fprintf(stderr, "Failed to allocate CPU memory\n");
        exit(EXIT_FAILURE);
    }

    // 初始化寄存器和内存
    memset(CHIP8_CPU, 0, sizeof(chip8_cpu_t));

    // 初始化程序计数器
    CHIP8_CPU->pc = PROGRAM_START_ADDR;

    // 加载字体集到内存
    memcpy(CHIP8_CPU->memory + FONTSET_START_ADDR, FONTSET, sizeof(FONTSET));

    // 随机数种子
    srand(time(NULL));

    CHIP8_CPU->draw_flag = 0;
}

void destroy(void)
{
    if (CHIP8_CPU) {
        free(CHIP8_CPU);
        CHIP8_CPU = NULL;
    }
}

int loadrom(const char* rom_path)
{
    if (!rom_path) return -1;

    FILE* rom_file = fopen(rom_path, "rb");
    if (!rom_file) {
        fprintf(stderr, "Failed to open ROM file: %s\n", rom_path);
        return -1;
    }

    // 获取ROM大小
    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);

    // 检查ROM大小是否超出内存限制
    if (rom_size > (4096 - PROGRAM_START_ADDR)) {
        fprintf(stderr, "ROM file too large\n");
        fclose(rom_file);
        return -1;
    }

    // 读取ROM到内存
    size_t bytes_read = fread(CHIP8_CPU->memory + PROGRAM_START_ADDR, 1, rom_size, rom_file);
    fclose(rom_file);

    if (bytes_read != rom_size) {
        fprintf(stderr, "Failed to read full ROM\n");
        return -1;
    }

    return 0;
}

// 定时器频率：60Hz，CPU默认每秒执行540条指令（9个cycle/帧）
#define CYCLES_PER_FRAME 9
static int cycle_count = 0;

void cycle(void)
{
    // 1. 获取指令（16位）
    CHIP8_CPU->opcode = (CHIP8_CPU->memory[CHIP8_CPU->pc] << 8) | CHIP8_CPU->memory[CHIP8_CPU->pc + 1];

    // 2. 递增PC（先递增，指令执行时可能修改）
    CHIP8_CPU->pc += 2;

    // 3. 解码并执行指令
    oc_exec();

    // 4. 更新定时器（每60帧更新一次，对应60Hz）
    cycle_count++;
    if (cycle_count >= CYCLES_PER_FRAME) {
        if (CHIP8_CPU->delayTimer > 0) {
            CHIP8_CPU->delayTimer--;
        }

        if (CHIP8_CPU->soundTimer > 0) {
            CHIP8_CPU->soundTimer--;
            // 如果soundTimer>0，触发蜂鸣
            if (CHIP8_CPU->soundTimer == 0) {
                audio_beep();
            }
        }
        cycle_count = 0;
    }
}
