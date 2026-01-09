#ifndef CHIP8_PLATFORM_H_
#define CHIP8_PLATFORM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// 显示参数
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCALE 10                  // 屏幕缩放倍数（最终窗口640x320）
#define WINDOW_WIDTH (SCREEN_WIDTH * SCALE)
#define WINDOW_HEIGHT (SCREEN_HEIGHT * SCALE)

// 全局SDL资源声明
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern TTF_Font* font;            // 用于显示速度百分比的字体

// 平台层函数声明
void display_init(void);          // 初始化SDL显示/字体
void display_update(void);        // 更新屏幕显示（含速度百分比）
void display_destroy(void);       // 释放SDL资源
void input_detect(void);          // 检测键盘输入（含速度调节）
void audio_init(void);            // 初始化音频（简化实现）
void audio_destroy(void);         // 释放音频资源
void audio_beep(void);            // 蜂鸣音效（简化实现）

#endif
