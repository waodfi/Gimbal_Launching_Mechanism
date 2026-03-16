#include <Arduino.h>
#include "Config.h"
#include "GimbalModule.h"
#include "LauncherModule.h"

// 当前系统的运行模式
SystemMode currentMode;

// 记录比赛模式运行状态
int currentTargetIndex = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("System Booting...");
    
    // 初始化模式切换引脚
    pinMode(PIN_MODE_SWITCH, INPUT_PULLUP);
    
    // 初始化子模块
    Gimbal_Init();
    Launcher_Init();
    
    // 从EEPROM加载历史保存坐标
    Gimbal_LoadTargetsFromEEPROM();
    
    // 首次读取模式
    bool isCompetition = (digitalRead(PIN_MODE_SWITCH) == LOW);
    currentMode = isCompetition ? MODE_COMPETITION : MODE_TUNING;
    
    Serial.print("Initial Mode: ");
    Serial.println(isCompetition ? "COMPETITION" : "TUNING");
}

void loop() {
    // 动态检查当前的模式
    bool isCompetition = (digitalRead(PIN_MODE_SWITCH) == LOW); // 假如拨动到底部变为低电平(比赛模式)
    SystemMode newMode = isCompetition ? MODE_COMPETITION : MODE_TUNING;
    
    // 如果发生了模式切换
    if (newMode != currentMode) {
        currentMode = newMode;
        Serial.print("Switched Mode: ");
        Serial.println(isCompetition ? "COMPETITION" : "TUNING");
        
        // 模式切换时，可以加入归零/重置逻辑
        if (currentMode == MODE_COMPETITION) {
            currentTargetIndex = 0; // 比赛开始，从第1个目标开始
        } else {
            // 切回调试模式时，有可能需要重新加载目标等清理操作
            Gimbal_LoadTargetsFromEEPROM(); 
        }
    }

    // 执行对应模式下的逻辑
    if (currentMode == MODE_TUNING) {
        // [调试模式] - 人工摇杆操控云台，按摇杆按键保存当点坐标
        Gimbal_RunTuningMode();
        
    } else {
        // [比赛模式] - 只要点没打完，就依次执行
        if (currentTargetIndex < Gimbal_GetTargetCount()) {
            Serial.print("Moving to Target: ");
            Serial.println(currentTargetIndex + 1);
            
            // 步骤 1: 云台移动到指定位置并等待稳定
            TargetPosition target = Gimbal_GetTarget(currentTargetIndex);
            Gimbal_MoveToTarget(target);
            delay(500); // 等待云台完全稳定
            
            // 步骤 2: 射击
            Launcher_Fire();
            delay(200); 
            
            // 步骤 3: 归位装弹
            Launcher_FeedBullet();
            delay(200);
            
            currentTargetIndex++; 
        } else {
            // 所有目标坐标都已完毕
            // 停机等待或者重置循环 (由你决定比赛模式下打完是停止循环还是怎么做)
        }
    }
}
