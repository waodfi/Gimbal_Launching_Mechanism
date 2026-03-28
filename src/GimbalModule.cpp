#include "GimbalModule.h"
#include <Servo.h>
#include <EEPROM.h>

// 云台所需的全局对象
Servo servoYaw;
Servo servoPitch;

constexpr int STARTUP_YAW = 90;
constexpr int STARTUP_PITCH = 90;

TargetPosition targets[MAX_TARGETS];
int savedTargetCount = 0;

// 当前云台角度（初始为搭弹位置）
int currentYaw = STARTUP_YAW;
int currentPitch = STARTUP_PITCH;

// EEPROM 地址分配
const int EEPROM_ADDR_COUNT = 0; // 第0个字节存坐标数量
const int EEPROM_ADDR_DATA = 1;  // 后续字节存坐标数组(结构体数组)的起始地址

void Gimbal_Init() {
    servoYaw.attach(PIN_SERVO_YAW);
    servoPitch.attach(PIN_SERVO_PITCH);
    
    // 摇杆引脚初始化
    pinMode(PIN_JOY_BTN, INPUT_PULLUP);
    
    Serial.println(F("Gimbal Module Initialized."));
    
    // 初始化时云台先回中位，避免开机冲到历史极限角
    servoYaw.write(STARTUP_YAW);
    servoPitch.write(STARTUP_PITCH);
}

void Gimbal_LoadTargetsFromEEPROM() {
    // 从第0号字节读取存了几个点位
    EEPROM.get(EEPROM_ADDR_COUNT, savedTargetCount);
    
    // 数据合法性校验，如果是一块崭新的Arduino(EEPROM里面全是255乱码)，直接清零
    if (savedTargetCount < 0 || savedTargetCount > MAX_TARGETS) {
        savedTargetCount = 0;
        EEPROM.put(EEPROM_ADDR_COUNT, savedTargetCount);
        Serial.println(F("EEPROM First Time Setup. Target count initialized to 0."));
    } else {
        Serial.print(F("Loaded "));
        Serial.print(savedTargetCount);
        Serial.println(F(" targets from EEPROM:"));
        
        // 读取具体的坐标数组
        for (int i = 0; i < savedTargetCount; i++) {
            EEPROM.get(EEPROM_ADDR_DATA + i * sizeof(TargetPosition), targets[i]);
            Serial.print(F(" Target ["));
            Serial.print(i + 1);
            Serial.print(F("]: Yaw="));
            Serial.print(targets[i].yawAngle);
            Serial.print(F(", Pitch="));
            Serial.println(targets[i].pitchAngle);
        }
    }
}

int Gimbal_GetTargetCount() {
    return savedTargetCount;
}

TargetPosition Gimbal_GetTarget(int index) {
    if(index >= 0 && index < savedTargetCount) {
        return targets[index];
    }
    return {LOAD_YAW, LOAD_PITCH}; // 如果索引越界，返回搭弹位置保护值
}

void Gimbal_UpdateTarget(int index, TargetPosition target) {
    if (index >= 0 && index < MAX_TARGETS) {
        targets[index] = target;
        if (index >= savedTargetCount) {
            savedTargetCount = index + 1;
            EEPROM.put(EEPROM_ADDR_COUNT, savedTargetCount);
        }
        
        // 保存到 EEPROM
        int addr = EEPROM_ADDR_DATA + index * sizeof(TargetPosition);
        EEPROM.put(addr, target);
        
        // 立即读回验证
        TargetPosition verify;
        EEPROM.get(addr, verify);
        Serial.print(F("[EEPROM] Saved Target["));
        Serial.print(index);
        Serial.print(F("] at addr "));
        Serial.print(addr);
        Serial.print(F(": Yaw="));
        Serial.print(target.yawAngle);
        Serial.print(F(", Pitch="));
        Serial.print(target.pitchAngle);
        Serial.print(F(" | Verified: Yaw="));
        Serial.print(verify.yawAngle);
        Serial.print(F(", Pitch="));
        Serial.println(verify.pitchAngle);
    }
}

void Gimbal_MoveToTarget(TargetPosition target) {
    currentYaw = target.yawAngle;
    currentPitch = target.pitchAngle;
    servoYaw.write(target.yawAngle);
    servoPitch.write(target.pitchAngle);
}

void Gimbal_MoveToTargetSmooth(TargetPosition target) {
    // 平滑移动：每次移动1度，延时20ms
    while (currentYaw != target.yawAngle || currentPitch != target.pitchAngle) {
        if (currentYaw < target.yawAngle) {
            currentYaw++;
        } else if (currentYaw > target.yawAngle) {
            currentYaw--;
        }
        
        if (currentPitch < target.pitchAngle) {
            currentPitch++;
        } else if (currentPitch > target.pitchAngle) {
            currentPitch--;
        }
        
        servoYaw.write(currentYaw);
        servoPitch.write(currentPitch);
        delay(20);
    }
}

void Gimbal_ClearEEPROM() {
    savedTargetCount = 0;
    EEPROM.put(EEPROM_ADDR_COUNT, savedTargetCount);
    Serial.println(F("[EEPROM] All saved targets cleared. Count reset to 0."));
}
