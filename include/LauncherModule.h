#pragma once
#include "Config.h"

// 负责初始化发射和供弹电机、外挂中断等
void Launcher_Init();

// 供弹电机：执行一次完整的供弹逻辑（顺时针/逆时针转动特定角度）
void Launcher_FeedBullet();

// 发射电机：执行一次发射逻辑（单向转圈，拉齿条发射）
void Launcher_Fire();

// 供调试使用：控制发射电机转动指定角度（正数正转，负数反转）
void Launcher_RotateLaunchMotor(int angle);

// 供调试使用：控制供弹电机转动指定角度（正数正转，负数反转）
void Launcher_RotateFeedMotor(int angle);

// 供诊断使用：开环强制让电机转动指定时间（毫秒），不依赖编码器
void Launcher_TestMotorOpenLoop(int motorId, int durationMs);
