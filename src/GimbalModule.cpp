#include "GimbalModule.h"
#include <Servo.h>
#include <EEPROM.h>

// 云台所需的全局对象
Servo servoYaw;
Servo servoPitch;

TargetPosition targets[MAX_TARGETS];
int savedTargetCount = 0;

// 当前云台角度（初始为 90 度居中）
int currentYaw = 90;
int currentPitch = 90;

// 摇杆按键状态与防抖参数
bool lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// EEPROM 地址分配
const int EEPROM_ADDR_COUNT = 0; // 第0个字节存坐标数量
const int EEPROM_ADDR_DATA = 1;  // 后续字节存坐标数组(结构体数组)的起始地址

void Gimbal_Init() {
    servoYaw.attach(PIN_SERVO_YAW);
    servoPitch.attach(PIN_SERVO_PITCH);
    
    // 摇杆引脚初始化 (将选定的A口作为纯数字引脚使用，采用内部上拉)
    pinMode(PIN_JOY_UP, INPUT_PULLUP);
    pinMode(PIN_JOY_DOWN, INPUT_PULLUP);
    pinMode(PIN_JOY_LEFT, INPUT_PULLUP);
    pinMode(PIN_JOY_RIGHT, INPUT_PULLUP);
    pinMode(PIN_JOY_BTN, INPUT_PULLUP);
    
    Serial.println("Gimbal Module Initialized.");
    
    // 初始化时云台回中保护
    servoYaw.write(currentYaw);
    servoPitch.write(currentPitch);
}

void Gimbal_RunTuningMode() {
    // 1. 读取开关量摇杆的数字状态 (低电平表示按下)
    bool isUp    = (digitalRead(PIN_JOY_UP) == LOW);
    bool isDown  = (digitalRead(PIN_JOY_DOWN) == LOW);
    bool isLeft  = (digitalRead(PIN_JOY_LEFT) == LOW);
    bool isRight = (digitalRead(PIN_JOY_RIGHT) == LOW);

    // 2. 转化为角度增量 (类似于键盘 WASD 控制)
    // 注意：因舵机安装方向差异，这里如果是反向的，到时候把 += 改成 -= 即可
    if (isLeft)  currentYaw += 1;
    if (isRight) currentYaw -= 1;
    
    if (isUp)    currentPitch += 1;
    if (isDown)  currentPitch -= 1;

    // 3. 限制角度范围在这合理区域内 (0 - 180度)
    currentYaw = constrain(currentYaw, 0, 180);
    currentPitch = constrain(currentPitch, 0, 180);

    // 4. 控制舵机实时跟随机摇杆移动
    servoYaw.write(currentYaw);
    servoPitch.write(currentPitch);

    // 5. 按键检测与坐标保存 (带外延防抖的下降沿检测)
    int reading = digitalRead(PIN_JOY_BTN);
    if (reading != lastBtnState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        // 如果是从高电平变为了低电平（按下了按键）
        if (reading == LOW && lastBtnState == HIGH) {
            
            // 如果点数已经存满了5个，再次按下按键则清空之前所有的记录，重新开始录入
            if (savedTargetCount >= MAX_TARGETS) {
                savedTargetCount = 0;
                EEPROM.put(EEPROM_ADDR_COUNT, savedTargetCount);
                Serial.println("Target list reset. Ready to record new targets.");
            }

            // 保存当前云台位置
            targets[savedTargetCount].yawAngle = currentYaw;
            targets[savedTargetCount].pitchAngle = currentPitch;
            savedTargetCount++;
            
            // 将数据持久化到 EEPROM 中
            EEPROM.put(EEPROM_ADDR_COUNT, savedTargetCount);
            // 每次直接把最新录入的那个结构体更新进对应的内存地址
            EEPROM.put(EEPROM_ADDR_DATA + (savedTargetCount - 1) * sizeof(TargetPosition), targets[savedTargetCount - 1]);
            
            Serial.print("Target Saved [");
            Serial.print(savedTargetCount);
            Serial.print(" / ");
            Serial.print(MAX_TARGETS);
            Serial.print("]: Yaw=");
            Serial.print(currentYaw);
            Serial.print(", Pitch=");
            Serial.println(currentPitch);
        }
    }
    // 刷新长按状态记录
    lastBtnState = reading;

    // 稍微延时，让舵机每次1度的微调看起来更平滑
    delay(15);
}

void Gimbal_LoadTargetsFromEEPROM() {
    // 从第0号字节读取存了几个点位
    EEPROM.get(EEPROM_ADDR_COUNT, savedTargetCount);
    
    // 数据合法性校验，如果是一块崭新的Arduino(EEPROM里面全是255乱码)，直接清零
    if (savedTargetCount < 0 || savedTargetCount > MAX_TARGETS) {
        savedTargetCount = 0;
        EEPROM.put(EEPROM_ADDR_COUNT, savedTargetCount);
        Serial.println("EEPROM First Time Setup. Target count initialized to 0.");
    } else {
        Serial.print("Loaded ");
        Serial.print(savedTargetCount);
        Serial.println(" targets from EEPROM:");
        
        // 读取具体的坐标数组
        for (int i = 0; i < savedTargetCount; i++) {
            EEPROM.get(EEPROM_ADDR_DATA + i * sizeof(TargetPosition), targets[i]);
            Serial.print(" Target [");
            Serial.print(i + 1);
            Serial.print("]: Yaw=");
            Serial.print(targets[i].yawAngle);
            Serial.print(", Pitch=");
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
    return {90, 90}; // 如果索引越界，返回回中保护值
}

void Gimbal_MoveToTarget(TargetPosition target) {
    servoYaw.write(target.yawAngle);
    servoPitch.write(target.pitchAngle);
}
