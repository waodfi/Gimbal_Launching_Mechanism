#pragma once
#include <Arduino.h>

// ===== 硬件引脚分配 =====
// 拨动开关 (选择模式) - 原 D4 引脚损坏，现改用 D8
#define PIN_MODE_SWITCH 8

// 摇杆控制 (模拟量摇杆：VRX, VRY, SW_KEY)
#define PIN_JOY_X A0
#define PIN_JOY_Y A1
#define PIN_JOY_BTN 5

// 两个舵机 (云台 Yaw/Pitch)
#define PIN_SERVO_YAW 9
#define PIN_SERVO_PITCH 10

// 供弹 N20 编码器 (挂外部中断1)
#define PIN_FEED_ENCA 3
#define PIN_FEED_ENCB 7

// 发射 N20 编码器 (挂外部中断0)
#define PIN_LAUNCH_ENCA 2
#define PIN_LAUNCH_ENCB 6

// ===== 核心状态机状态定义 =====
enum SystemMode {
  MODE_TUNING = 0,     // 调试/定位模式 (3分钟)
  MODE_COMPETITION = 1 // 比赛/自动模式 (4分钟)
};

// ===== 存储结构定义 =====
struct TargetPosition {
  int yawAngle;
  int pitchAngle;
};

#define MAX_TARGETS 5
