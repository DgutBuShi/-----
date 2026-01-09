#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8_cpu.h"
#include "chip8_opcodes.h"

// 辅助宏：快速提取指令中的位段
#define Vx (CHIP8_CPU->registers[(CHIP8_CPU->opcode & 0x0F00) >> 8])
#define Vy (CHIP8_CPU->registers[(CHIP8_CPU->opcode & 0x00F0) >> 4])
#define nnn (CHIP8_CPU->opcode & 0x0FFF)
#define nn (CHIP8_CPU->opcode & 0x00FF)
#define n (CHIP8_CPU->opcode & 0x000F)
#define x ((CHIP8_CPU->opcode & 0x0F00) >> 8)
#define y ((CHIP8_CPU->opcode & 0x00F0) >> 4)

// 指令分发：解码并执行对应指令
void oc_exec(void)
{
    switch (CHIP8_CPU->opcode & 0xF000)
    {
    case 0x0000:
        switch (CHIP8_CPU->opcode & 0x00FF)
        {
        case 0x00E0: oc_00e0(); break;
        case 0x00EE: oc_00ee(); break;
        default: oc_null(); break;
        }
        break;
    case 0x1000: oc_1nnn(); break;
    case 0x2000: oc_2nnn(); break;
    case 0x3000: oc_3xnn(); break;
    case 0x4000: oc_4xnn(); break;
    case 0x5000: oc_5xy0(); break;
    case 0x6000: oc_6xnn(); break;
    case 0x7000: oc_7xnn(); break;
    case 0x8000:
        switch (CHIP8_CPU->opcode & 0x000F)
        {
        case 0x0: oc_8xy0(); break;
        case 0x1: oc_8xy1(); break;
        case 0x2: oc_8xy2(); break;
        case 0x3: oc_8xy3(); break;
        case 0x4: oc_8xy4(); break;
        case 0x5: oc_8xy5(); break;
        case 0x6: oc_8xy6(); break;
        case 0x7: oc_8xy7(); break;
        case 0xE: oc_8xye(); break;
        default: oc_null(); break;
        }
        break;
    case 0x9000: oc_9xy0(); break;
    case 0xA000: oc_annn(); break;
    case 0xB000: oc_bxnn(); break;
    case 0xC000: oc_cxnn(); break;
    case 0xD000: oc_dxyn(); break;
    case 0xE000:
        switch (CHIP8_CPU->opcode & 0x00FF)
        {
        case 0x9E: oc_ex9e(); break;
        case 0xA1: oc_exa1(); break;
        default: oc_null(); break;
        }
        break;
    case 0xF000:
        switch (CHIP8_CPU->opcode & 0x00FF)
        {
        case 0x07: oc_fx07(); break;
        case 0x0A: oc_fx0a(); break;
        case 0x15: oc_fx15(); break;
        case 0x18: oc_fx18(); break;
        case 0x1E: oc_fx1e(); break;
        case 0x29: oc_fx29(); break;
        case 0x33: oc_fx33(); break;
        case 0x55: oc_fx55(); break;
        case 0x65: oc_fx65(); break;
        default: oc_null(); break;
        }
        break;
    default:
        oc_null();
        break;
    }
}

// 未知指令处理
void oc_null(void)
{
    printf("[STATE][OPCODE] Unknown opcode: 0x%04X\n", CHIP8_CPU->opcode);
}

// 00E0: 清屏
void oc_00e0(void) {
    memset(CHIP8_CPU->video, 0, sizeof(CHIP8_CPU->video));
    CHIP8_CPU->draw_flag = 1;
}

// 00EE: 从子程序返回
void oc_00ee(void) {
    CHIP8_CPU->sp--;
    CHIP8_CPU->pc = CHIP8_CPU->stack[CHIP8_CPU->sp];
}

// 1nnn: 跳转到地址nnn
void oc_1nnn(void) {
    CHIP8_CPU->pc = nnn;
}

// 2nnn: 调用子程序nnn
void oc_2nnn(void) {
    CHIP8_CPU->stack[CHIP8_CPU->sp] = CHIP8_CPU->pc;
    CHIP8_CPU->sp++;
    CHIP8_CPU->pc = nnn;
}

// 3xnn: 若Vx == nn则跳过下一条指令
void oc_3xnn(void) {
    if (Vx == nn) {
        CHIP8_CPU->pc += 2;
    }
}

// 4xnn: 若Vx != nn则跳过下一条指令
void oc_4xnn(void) {
    if (Vx != nn) {
        CHIP8_CPU->pc += 2;
    }
}

// 5xy0: 若Vx == Vy则跳过下一条指令
void oc_5xy0(void) {
    if (Vx == Vy) {
        CHIP8_CPU->pc += 2;
    }
}

// 6xnn: Vx = nn
void oc_6xnn(void) {
    Vx = nn;
}

// 7xnn: Vx += nn
void oc_7xnn(void) {
    Vx += nn;
}

// 8xy0: Vx = Vy
void oc_8xy0(void) {
    Vx = Vy;
}

// 8xy1: Vx |= Vy
void oc_8xy1(void) {
    Vx |= Vy;
}

// 8xy2: Vx &= Vy
void oc_8xy2(void) {
    Vx &= Vy;
}

// 8xy3: Vx ^= Vy
void oc_8xy3(void) {
    Vx ^= Vy;
}

// 8xy4: Vx += Vy (带进位)
void oc_8xy4(void) {
    uint16_t result = Vx + Vy;
    CHIP8_CPU->registers[0xF] = (result > 0xFF) ? 1 : 0;
    Vx = result & 0xFF;
}

// 8xy5: Vx -= Vy (带借位)
void oc_8xy5(void) {
    CHIP8_CPU->registers[0xF] = (Vx > Vy) ? 1 : 0;
    Vx -= Vy;
}

// 8xy6: Vx >>= 1 (保留最低位到VF)
void oc_8xy6(void) {
    CHIP8_CPU->registers[0xF] = Vx & 0x01;
    Vx >>= 1;
}

// 8xy7: Vx = Vy - Vx (带借位)
void oc_8xy7(void) {
    CHIP8_CPU->registers[0xF] = (Vy > Vx) ? 1 : 0;
    Vx = Vy - Vx;
}

// 8xye: Vx <<= 1 (保留最高位到VF)
void oc_8xye(void) {
    CHIP8_CPU->registers[0xF] = (Vx & 0x80) ? 1 : 0;
    Vx <<= 1;
}

// 9xy0: 若Vx != Vy则跳过下一条指令
void oc_9xy0(void) {
    if (Vx != Vy) {
        CHIP8_CPU->pc += 2;
    }
}

// Annn: I = nnn
void oc_annn(void) {
    CHIP8_CPU->index = nnn;
}

// Bxnn: 跳转到V0 + nnn
void oc_bxnn(void) {
    CHIP8_CPU->pc = CHIP8_CPU->registers[0] + nnn;
}

// Cxnn: Vx = 随机数 & nn
void oc_cxnn(void) {
    Vx = (rand() % 0xFF) & nn;
}

// Dxyn: 绘制Sprite (x, y, 高度n)
void oc_dxyn(void) {
    uint8_t x_pos = Vx % 64;
    uint8_t y_pos = Vy % 32;
    CHIP8_CPU->registers[0xF] = 0;

    for (uint8_t row = 0; row < n; row++) {
        uint8_t sprite_byte = CHIP8_CPU->memory[CHIP8_CPU->index + row];

        for (uint8_t col = 0; col < 8; col++) {
            uint8_t pixel = (sprite_byte >> (7 - col)) & 0x01;
            int pixel_idx = (y_pos + row) * 64 + (x_pos + col);

            if (pixel_idx >= 64 * 32) break;

            if (pixel) {
                if (CHIP8_CPU->video[pixel_idx]) {
                    CHIP8_CPU->registers[0xF] = 1; // 碰撞检测
                }
                CHIP8_CPU->video[pixel_idx] ^= 1; // 异或绘制
            }
        }
    }

    CHIP8_CPU->draw_flag = 1;
}

// Ex9E: 若按键Vx被按下则跳过下一条指令
void oc_ex9e(void) {
    if (CHIP8_CPU->keypad[Vx]) {
        CHIP8_CPU->pc += 2;
    }
}

// ExA1: 若按键Vx未被按下则跳过下一条指令
void oc_exa1(void) {
    if (!CHIP8_CPU->keypad[Vx]) {
        CHIP8_CPU->pc += 2;
    }
}

// Fx07: Vx = 延迟定时器值
void oc_fx07(void) {
    Vx = CHIP8_CPU->delayTimer;
}

// Fx0A: 等待按键并存储到Vx
void oc_fx0a(void) {
    int key_pressed = 0;
    for (int i = 0; i < 16; i++) {
        if (CHIP8_CPU->keypad[i]) {
            Vx = i;
            key_pressed = 1;
            break;
        }
    }
    if (!key_pressed) {
        CHIP8_CPU->pc -= 2; // 未按键则重复执行
    }
}

// Fx15: 延迟定时器 = Vx
void oc_fx15(void) {
    CHIP8_CPU->delayTimer = Vx;
}

// Fx18: 声音定时器 = Vx
void oc_fx18(void) {
    CHIP8_CPU->soundTimer = Vx;
}

// Fx1E: I += Vx
void oc_fx1e(void) {
    CHIP8_CPU->index += Vx;
}

// Fx29: I = 字体地址(Vx) (每个字体5字节)
void oc_fx29(void) {
    CHIP8_CPU->index = Vx * 5;
}

// Fx33: 存储Vx的BCD码到内存I/I+1/I+2
void oc_fx33(void) {
    CHIP8_CPU->memory[CHIP8_CPU->index] = Vx / 100;          // 百位
    CHIP8_CPU->memory[CHIP8_CPU->index + 1] = (Vx / 10) % 10; // 十位
    CHIP8_CPU->memory[CHIP8_CPU->index + 2] = Vx % 10;        // 个位
}

// Fx55: 存储V0-Vx到内存I
void oc_fx55(void) {
    for (int i = 0; i <= x; i++) {
        CHIP8_CPU->memory[CHIP8_CPU->index + i] = CHIP8_CPU->registers[i];
    }
    CHIP8_CPU->index += x + 1;
}

// Fx65: 从内存I加载V0-Vx
void oc_fx65(void) {
    for (int i = 0; i <= x; i++) {
        CHIP8_CPU->registers[i] = CHIP8_CPU->memory[CHIP8_CPU->index + i];
    }
    CHIP8_CPU->index += x + 1;
}
