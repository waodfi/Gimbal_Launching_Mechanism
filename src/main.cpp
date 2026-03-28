#include <Arduino.h>
#include <EEPROM.h>
#include "Config.h"
#include "GimbalModule.h"
#include "LauncherModule.h"

// 当前系统模式
SystemMode currentMode = MODE_TUNING;

// 状态机变量 - 调试模式
int tuningTargetIndex = 0;
bool hasBullet = true; // 假设初始已预装填第一个小球
bool tuningCompleted = false;

// 状态机变量 - 比赛模式
bool competitionStarted = false;
int competitionTargetIndex = 0;

// 按键防抖
bool testFireRawState = HIGH;
bool testFireStableState = HIGH;
unsigned long lastTestFireChangeTime = 0;
const unsigned long DEBOUNCE_MS = 50;

bool joyBtnRawState = HIGH;
bool joyBtnStableState = HIGH;
unsigned long lastJoyBtnChangeTime = 0;

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("========================================"));
    Serial.println(F("Gimbal Launching Mechanism 2.0"));
    Serial.println(F("========================================"));

    // 初始化引脚
    pinMode(PIN_MODE_SWITCH, INPUT_PULLUP);
    pinMode(PIN_TEST_FIRE_BTN, INPUT_PULLUP);
    pinMode(PIN_JOY_BTN, INPUT_PULLUP);
    
    // 初始化防抖状态以匹配当前的真实硬件电平
    testFireRawState = digitalRead(PIN_TEST_FIRE_BTN);
    testFireStableState = testFireRawState;
    joyBtnRawState = digitalRead(PIN_JOY_BTN);
    joyBtnStableState = joyBtnRawState;

    // 初始化子模块
    Gimbal_Init();
    Launcher_Init();

    // 从 EEPROM 加载保存的目标
    Gimbal_LoadTargetsFromEEPROM();

    // 初始化模式 - 临时强制启动为调试模式，绕过硬件开关问题
    currentMode = MODE_TUNING;
    Serial.println(F("Mode: TUNING (Forced)"));
    
    // 调试模式初始化：上电保持在中位，避免被历史点位拉到极限角
    tuningTargetIndex = 0;
    tuningCompleted = false;
    hasBullet = true;
    Gimbal_MoveToTarget({90, 90});
    
    /* 原始模式检测逻辑 - 硬件修复后恢复
    if (digitalRead(PIN_MODE_SWITCH) == LOW) {
        currentMode = MODE_COMPETITION;
        Serial.println(F("Mode: COMPETITION"));
    } else {
        currentMode = MODE_TUNING;
        Serial.println(F("Mode: TUNING"));
        
        tuningTargetIndex = 0;
        tuningCompleted = false;
        hasBullet = true;
        Gimbal_MoveToTarget({90, 90});
    }
    */
}

void loop() {
    // === 诊断心跳打印（每2秒打印一次） ===
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 2000) {
        lastHeartbeat = millis();
        Serial.print(F("[DIAG] Heartbeat. Mode: "));
        Serial.print(currentMode == MODE_TUNING ? F("TUNING") : F("COMPETITION"));
        Serial.print(F(" | TestFireBtn Raw: "));
        Serial.println(digitalRead(PIN_TEST_FIRE_BTN));
    }

    // === 硬件调试专用代码，测试完毕后请注释掉 ===
    /*
    Serial.print("D12 (Fire Btn): ");
    Serial.print(digitalRead(PIN_TEST_FIRE_BTN));
    Serial.print(" | D5 (Joy Btn): ");
    Serial.println(digitalRead(PIN_JOY_BTN));
    delay(200);
    return;
    */
    // =====================================

    // 1. 检查模式切换开关
    SystemMode readMode = (digitalRead(PIN_MODE_SWITCH) == LOW) ? MODE_COMPETITION : MODE_TUNING;
    if (readMode != currentMode) {
        currentMode = readMode;
        if (currentMode == MODE_COMPETITION) {
            Serial.println(F("Switched to COMPETITION mode."));
            competitionStarted = false;
            competitionTargetIndex = 0;
        } else {
            Serial.println(F("Switched to TUNING mode."));
            tuningTargetIndex = 0;
            tuningCompleted = false;
            hasBullet = true; // 假设每次切回调试模式时，用户会预装填
            Gimbal_MoveToTarget({90, 90}); // 切换到调试模式时，云台回到中位，避免极限角度
        }
        delay(500); // 防抖和模式切换缓冲
    }

    // 处理串口输入以微调电机（格式：电机编号 角度）
    // 1: 蓄力/发射电机, 2: 供弹电机
    // 例如："1 180"、"2 -180"
    static char serialCmdBuf[24];
    static uint8_t serialCmdLen = 0;
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            serialCmdBuf[serialCmdLen] = '\0';

            if (serialCmdLen > 0) {
                char *p = serialCmdBuf;
                char *end1 = nullptr;
                const long motorId = strtol(p, &end1, 10);
                while (*end1 == ' ') {
                    end1++;
                }

                char *end2 = nullptr;
                const long inputAngle = strtol(end1, &end2, 10);
                while (*end2 == ' ') {
                    end2++;
                }

                const bool hasTwoNumbers = (end1 != end2);
                const bool noExtraChars = (*end2 == '\0');

                // 检查是否是 CLEAR 命令
                if (strcmp(serialCmdBuf, "CLEAR") == 0 || strcmp(serialCmdBuf, "clear") == 0) {
                    Gimbal_ClearEEPROM();
                } else if (hasTwoNumbers && noExtraChars && inputAngle != 0 && motorId == 1) {
                    Launcher_RotateLaunchMotor(static_cast<int>(inputAngle));
                } else if (hasTwoNumbers && noExtraChars && inputAngle != 0 && motorId == 2) {
                    Launcher_RotateFeedMotor(static_cast<int>(inputAngle));
                } else if (hasTwoNumbers && noExtraChars && motorId == 99) {
                    Launcher_TestMotorOpenLoop(static_cast<int>(inputAngle), 500);
                } else {
                    Serial.println(F("[SERIAL] Invalid command. Use: <1|2> <angle>, e.g. 1 180 or 2 -180"));
                    Serial.println(F("[SERIAL] Diag command: 99 <1|2> (Open loop test)"));
                    Serial.println(F("[SERIAL] Clear EEPROM: CLEAR"));
                }
            }

            serialCmdLen = 0;
        } else {
            if (serialCmdLen < sizeof(serialCmdBuf) - 1) {
                serialCmdBuf[serialCmdLen++] = c;
            } else {
                serialCmdLen = 0;
                Serial.println(F("[SERIAL] Command too long."));
            }
        }
    }

    if (currentMode == MODE_TUNING) {
        if (tuningCompleted) {
            return;
        }

        int joyX = analogRead(PIN_JOY_X);
        int joyY = analogRead(PIN_JOY_Y);
        int deadZone = 100;
        int center = 512;
        
        extern int currentYaw;
        extern int currentPitch;
        
        bool moved = false;
        if (joyX < center - deadZone) { currentYaw += 1; moved = true; }
        if (joyX > center + deadZone) { currentYaw -= 1; moved = true; }
        if (joyY < center - deadZone) { currentPitch += 1; moved = true; }
        if (joyY > center + deadZone) { currentPitch -= 1; moved = true; }
        
        if (moved) {
            currentYaw = constrain(currentYaw, 0, 180);
            currentPitch = constrain(currentPitch, 0, 180);
            TargetPosition t = {currentYaw, currentPitch};
            Gimbal_MoveToTarget(t);
        }
        delay(15);

        const bool btnNow = digitalRead(PIN_TEST_FIRE_BTN);
        
        if (btnNow != testFireRawState) {
            lastTestFireChangeTime = millis();
            testFireRawState = btnNow;
        }

        if ((millis() - lastTestFireChangeTime) > DEBOUNCE_MS) {
            if (btnNow != testFireStableState) {
                Serial.print(F("[BTN] 稳定状态变化: "));
                Serial.println(btnNow ? F("HIGH") : F("LOW"));
                
                if (testFireStableState == LOW && btnNow == HIGH) {
                    if (hasBullet) {
                        Serial.println(F("[TUNING] Firing..."));
                        Launcher_Fire();
                        hasBullet = false;
                    } else {
                        Serial.println(F("[TUNING] Loading next bullet..."));
                        TargetPosition temp = {currentYaw, currentPitch};
                        
                        Gimbal_MoveToTargetSmooth({LOAD_YAW, LOAD_PITCH});
                        delay(2000); // 等待云台移动到搭弹位置并稳定
                        
                        Launcher_FeedBullet();
                        delay(2000); // 等待球落到发射区
                        
                        Gimbal_MoveToTargetSmooth(temp);
                        delay(2000); // 等待云台回到瞄准位置并稳定
                        hasBullet = true;
                    }
                }
                
                testFireStableState = btnNow;
            }
        }

        bool joyBtnReading = digitalRead(PIN_JOY_BTN);
        if (joyBtnReading != joyBtnRawState) {
            lastJoyBtnChangeTime = millis();
            joyBtnRawState = joyBtnReading;
        }

        if ((millis() - lastJoyBtnChangeTime) > DEBOUNCE_MS) {
            if (joyBtnReading != joyBtnStableState) {
                if (joyBtnStableState == HIGH && joyBtnReading == LOW) {
                    TargetPosition t = {currentYaw, currentPitch};
                    Gimbal_UpdateTarget(tuningTargetIndex, t);
                    
                    Serial.print(F("[TUNING] Target "));
                    Serial.print(tuningTargetIndex + 1);
                    Serial.println(F(" saved."));

                    tuningTargetIndex++;
                    if (tuningTargetIndex < MAX_TARGETS) {
                        Serial.println(F("[TUNING] Loading bullet for next target..."));
                        Gimbal_MoveToTargetSmooth({LOAD_YAW, LOAD_PITCH});
                        delay(2000); // 等待云台移动到搭弹位置并稳定
                        
                        Launcher_FeedBullet();
                        delay(2000); // 等待球落到发射区
                        hasBullet = true;
                        
                        TargetPosition nextTarget = Gimbal_GetTarget(tuningTargetIndex);
                        Gimbal_MoveToTargetSmooth(nextTarget);
                        delay(2000); // 等待云台移动到下一个目标位置并稳定
                    } else {
                        Serial.println(F("[TUNING] All 5 targets updated. Tuning completed."));
                        tuningCompleted = true;
                    }
                }
                joyBtnStableState = joyBtnReading;
            }
        }

    } else if (currentMode == MODE_COMPETITION) {
        if (!competitionStarted) {
            bool joyBtnReading = digitalRead(PIN_JOY_BTN);
            if (joyBtnReading != joyBtnRawState) {
                lastJoyBtnChangeTime = millis();
                joyBtnRawState = joyBtnReading;
            }

            if ((millis() - lastJoyBtnChangeTime) > DEBOUNCE_MS) {
                if (joyBtnReading != joyBtnStableState) {
                    if (joyBtnStableState == HIGH && joyBtnReading == LOW) {
                        Serial.println(F("[COMPETITION] Match started!"));
                        competitionStarted = true;
                        competitionTargetIndex = 0;
                    }
                    joyBtnStableState = joyBtnReading;
                }
            }
        } else {
            if (competitionTargetIndex < Gimbal_GetTargetCount() && competitionTargetIndex < MAX_TARGETS) {
                Serial.print(F("[COMPETITION] Processing Target "));
                Serial.println(competitionTargetIndex + 1);

                Gimbal_MoveToTargetSmooth({LOAD_YAW, LOAD_PITCH});
                delay(2000); // 等待云台移动到搭弹位置并稳定

                Launcher_FeedBullet();
                delay(2000); // 等待球落到发射区

                TargetPosition target = Gimbal_GetTarget(competitionTargetIndex);
                Gimbal_MoveToTargetSmooth(target);
                delay(2000); // 等待云台瞄准稳定

                Launcher_Fire();
                delay(500);

                competitionTargetIndex++;
            } else {
                Serial.println(F("[COMPETITION] All targets fired. Match ended."));
                while(digitalRead(PIN_MODE_SWITCH) == LOW) {
                    delay(100);
                }
            }
        }
    }
}
