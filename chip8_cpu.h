#ifndef CHIP8_CPU_H_ 
#define CHIP8_CPU_H_ 
#define CYCLES_PER_FRAME 9 // 每帧执行9个CPU周期，对应60Hz帧率

#include <stdint.h>

#define FONTSET_START_ADDR 0x000
#define PROGRAM_START_ADDR 0x200

typedef struct {
    uint8_t registers[16];
    uint8_t memory[4096];
    uint16_t index;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint8_t keypad[16];
    uint8_t video[64 * 32];
    uint16_t opcode;
    int draw_flag; // 标记是否需要刷新屏幕
} chip8_cpu_t;

extern chip8_cpu_t* CHIP8_CPU;

int loadrom(const char* rom);
void init(void);
void destroy(void);
void cycle(void);

#endif
