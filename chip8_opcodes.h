#ifndef OPCODES_H
#define OPCODES_H

#include "chip8_cpu.h"

// 指令函数声明
void oc_00e0(void);  // 清屏
void oc_00ee(void);  // 从子程序返回
void oc_1nnn(void);  // 跳转到地址nnn
void oc_2nnn(void);  // 调用子程序nnn
void oc_3xnn(void);  // 若Vx==nn则跳过下一条指令
void oc_4xnn(void);  // 若Vx!=nn则跳过下一条指令
void oc_5xy0(void);  // 若Vx==Vy则跳过下一条指令
void oc_6xnn(void);  // Vx = nn
void oc_7xnn(void);  // Vx += nn
void oc_8xy0(void);  // Vx = Vy
void oc_8xy1(void);  // Vx |= Vy
void oc_8xy2(void);  // Vx &= Vy
void oc_8xy3(void);  // Vx ^= Vy
void oc_8xy4(void);  // Vx += Vy (带进位)
void oc_8xy5(void);  // Vx -= Vy (带借位)
void oc_8xy6(void);  // Vx >>= 1 (保留最低位到VF)
void oc_8xy7(void);  // Vx = Vy - Vx (带借位)
void oc_8xye(void);  // Vx <<= 1 (保留最高位到VF)
void oc_9xy0(void);  // 若Vx!=Vy则跳过下一条指令
void oc_annn(void);  // I = nnn
void oc_bxnn(void);  // 跳转到V0+nnn
void oc_cxnn(void);  // Vx = 随机数 & nn
void oc_dxyn(void);  // 绘制Sprite
void oc_ex9e(void);  // 若按键Vx被按下则跳过下一条指令
void oc_exa1(void);  // 若按键Vx未被按下则跳过下一条指令
void oc_fx07(void);  // Vx = 延迟定时器值
void oc_fx0a(void);  // 等待按键并存储到Vx
void oc_fx15(void);  // 延迟定时器 = Vx
void oc_fx18(void);  // 声音定时器 = Vx
void oc_fx1e(void);  // I += Vx
void oc_fx29(void);  // I = 字体地址(Vx)
void oc_fx33(void);  // 存储Vx的BCD码到内存I/I+1/I+2
void oc_fx55(void);  // 存储V0-Vx到内存I
void oc_fx65(void);  // 从内存I加载V0-Vx

void oc_null(void);
void oc_exec(void);

#endif
