#pragma once
#include "Config.h"

// 负责初始化云台舵机与摇杆等硬件
void Gimbal_Init();

// 执行调试模式下的云台控制和EEPROM坐标保存操作
void Gimbal_RunTuningMode();

// 从EEPROM读取所有坐标并重置当前目标索引
void Gimbal_LoadTargetsFromEEPROM();

// 返回已保存的目标数量
int Gimbal_GetTargetCount();

// 获取指定索引的目标坐标
TargetPosition Gimbal_GetTarget(int index);

// 使云台移动到指定的坐标位置
void Gimbal_MoveToTarget(TargetPosition target);
