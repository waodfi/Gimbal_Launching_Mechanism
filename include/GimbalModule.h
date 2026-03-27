#pragma once
#include "Config.h"

// 负责初始化云台舵机与摇杆等硬件
void Gimbal_Init();

// 从EEPROM读取所有坐标并重置当前目标索引
void Gimbal_LoadTargetsFromEEPROM();

// 返回已保存的目标数量
int Gimbal_GetTargetCount();

// 获取指定索引的目标坐标
TargetPosition Gimbal_GetTarget(int index);

// 更新并保存指定索引的目标坐标到EEPROM
void Gimbal_UpdateTarget(int index, TargetPosition target);

// 使云台移动到指定的坐标位置
void Gimbal_MoveToTarget(TargetPosition target);
