#ifndef CHIP8_CPU_H_ 
#define CHIP8_CPU_H_ 

#include <stdint.h>

// 内存地址常量
#define FONTSET_START_ADDR 0x000
#define PROGRAM_START_ADDR 0x200
// 基准每帧执行周期数（对应540指令/秒，60Hz帧率）
#define BASE_CYCLES_PER_FRAME 9

// CHIP-8 CPU核心结构体
typedef struct {
    uint8_t registers[16];        // V0-VF通用寄存器
    uint8_t memory[4096];         // 4KB内存
    uint16_t index;               // 索引寄存器I
    uint16_t pc;                  // 程序计数器
    uint16_t stack[16];           // 栈（子程序返回地址）
    uint8_t sp;                   // 栈指针
    uint8_t delayTimer;           // 延迟定时器
    uint8_t soundTimer;           // 声音定时器
    uint8_t keypad[16];           // 16键键盘映射
    uint8_t video[64 * 32];         // 64x32显示缓冲区
    uint16_t opcode;              // 当前执行的16位指令
    int draw_flag;                // 屏幕刷新标记
} chip8_cpu_t;

// 全局变量声明
extern chip8_cpu_t* CHIP8_CPU;
extern float speed_coeff;        // 速度系数（0.5-2.0，1.0=100%基准速度）
extern int is_running;           // 程序运行标记

// 核心函数声明
int loadrom(const char* rom);    // 加载ROM文件
void init(void);                 // 初始化CPU（首次启动）
void reset(void);                // 重置CPU（加载新ROM时）
void destroy(void);              // 释放CPU内存
void cycle(void);                // 执行一次CPU周期

#endif
