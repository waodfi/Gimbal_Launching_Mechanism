#include "LauncherModule.h"
#include <Adafruit_MotorShield.h>

// I2C地址默认0x60
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *launchMotor = AFMS.getMotor(1); // M1: 发射电机
Adafruit_DCMotor *feedMotor = AFMS.getMotor(2);   // M2: 供弹电机

// ==========================================
// 供弹电机配置与参数 (从 feed_motor_debug 提取)
// ==========================================
constexpr uint8_t FEED_SPEED = 180;
constexpr long FEED_COUNTS_PER_360 = 8800;
constexpr int8_t FEED_DIRECTION = -1;
constexpr int FEED_TARGET_ANGLE = 60;
constexpr long FEED_POSITION_TOLERANCE = 4;
constexpr unsigned long FEED_TIMEOUT_MS = 6000;

// ==========================================
// 发射电机配置与参数 (从 launch_motor_debug 提取)
// ==========================================
constexpr uint8_t LAUNCH_SPEED = 255;
constexpr long LAUNCH_COUNTS_PER_360 = 3278;
constexpr int8_t LAUNCH_DIRECTION = 1;
constexpr int LAUNCH_TARGET_ANGLE = 2030;
constexpr long LAUNCH_POSITION_TOLERANCE = 4;
constexpr unsigned long LAUNCH_TIMEOUT_MS = 20000;

// ==========================================
// 编码器状态变量
// ==========================================
volatile long feedEncoderCount = 0;
volatile int8_t feedMotorDirection = 0;

volatile long launchEncoderCount = 0;
volatile int8_t launchMotorDirection = 0;

// ==========================================
// 中断服务函数
// ==========================================
void ISR_FeedEncoderA() {
    feedEncoderCount += feedMotorDirection; 
}

void ISR_LaunchEncoderA() {
    launchEncoderCount += launchMotorDirection;
}

// 辅助函数：原子读取供弹编码器
long readFeedCount() {
    noInterrupts();
    long v = feedEncoderCount;
    interrupts();
    return v;
}

// 辅助函数：原子读取发射编码器
long readLaunchCount() {
    noInterrupts();
    long v = launchEncoderCount;
    interrupts();
    return v;
}

void Launcher_Init() {
    Serial.println(F("Calling AFMS.begin()..."));
    if (!AFMS.begin()) {
        Serial.println(F("[ERROR] Motor Shield 初始化失败。请检查连线。"));
        while (1); 
    }
    Serial.println(F("AFMS.begin() success."));
    
    // 初始化编码器引脚
    pinMode(PIN_FEED_ENCA, INPUT_PULLUP);
    pinMode(PIN_LAUNCH_ENCA, INPUT_PULLUP);

    // 绑定外部中断 (D2=中断0=Launch, D3=中断1=Feed)
    attachInterrupt(digitalPinToInterrupt(PIN_LAUNCH_ENCA), ISR_LaunchEncoderA, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_FEED_ENCA), ISR_FeedEncoderA, RISING);

    // 初始状态停转
    launchMotor->setSpeed(0);
    launchMotor->run(RELEASE);
    feedMotor->setSpeed(0);
    feedMotor->run(RELEASE);

    Serial.println(F("Launcher Module Initialized."));
}

void Launcher_FeedBullet() {
    Serial.println(F("Action: FEED BULLET (Closed-loop)"));
    
    // 计算供弹需要的相对计数值
    const long targetCounts = (FEED_COUNTS_PER_360 * FEED_TARGET_ANGLE) / 360;
    
    const long startCount = readFeedCount();
    const long adjustedDelta = targetCounts * FEED_DIRECTION;
    const long target = startCount + adjustedDelta;
    
    feedMotorDirection = (adjustedDelta > 0) ? 1 : -1;
    feedMotor->setSpeed(FEED_SPEED);
    feedMotor->run((adjustedDelta > 0) ? FORWARD : BACKWARD);

    const unsigned long t0 = millis();
    while (true) {
        const long err = target - readFeedCount();
        if (labs(err) <= FEED_POSITION_TOLERANCE) {
            break;
        }
        if (millis() - t0 > FEED_TIMEOUT_MS) {
            Serial.println(F("[WARN] Feed motor timeout."));
            break;
        }
        delay(1);
    }

    // 到位后停转
    feedMotorDirection = 0;
    feedMotor->setSpeed(0);
    feedMotor->run(RELEASE);
    
    Serial.println(F("FEED Complete."));
}

void Launcher_Fire() {
    Serial.println(F("Action: FIRE (Closed-loop)"));
    
    // 计算发射需要的相对计数值
    const long targetCounts = (LAUNCH_COUNTS_PER_360 * LAUNCH_TARGET_ANGLE) / 360;
    
    const long startCount = readLaunchCount();
    const long adjustedDelta = targetCounts * LAUNCH_DIRECTION;
    const long target = startCount + adjustedDelta;
    
    launchMotorDirection = (adjustedDelta > 0) ? 1 : -1;
    launchMotor->setSpeed(LAUNCH_SPEED);
    launchMotor->run((adjustedDelta > 0) ? FORWARD : BACKWARD);

    const unsigned long t0 = millis();
    while (true) {
        const long err = target - readLaunchCount();
        if (labs(err) <= LAUNCH_POSITION_TOLERANCE) {
            break;
        }
        if (millis() - t0 > LAUNCH_TIMEOUT_MS) {
            Serial.println(F("[WARN] Launch motor timeout."));
            break;
        }
        delay(1);
    }

    // 到位后刹车停转
    launchMotorDirection = 0;
    launchMotor->setSpeed(0);
    launchMotor->run(RELEASE);
    
    Serial.println(F("FIRE Complete."));
}

void Launcher_RotateLaunchMotor(int angle) {
    if (angle == 0) return;
    
    Serial.print(F("Action: ROTATE LAUNCH MOTOR ("));
    Serial.print(angle);
    Serial.println(F(" deg)"));
    
    // 计算需要的相对计数值
    const long targetCounts = (LAUNCH_COUNTS_PER_360 * abs(angle)) / 360;
    
    const long startCount = readLaunchCount();
    // 如果传入正数，顺着 LAUNCH_DIRECTION 转，如果负数，反着转
    const int8_t requestedDir = (angle > 0) ? 1 : -1;
    const long adjustedDelta = targetCounts * (LAUNCH_DIRECTION * requestedDir);
    const long target = startCount + adjustedDelta;
    
    launchMotorDirection = (adjustedDelta > 0) ? 1 : -1;
    launchMotor->setSpeed(LAUNCH_SPEED);
    launchMotor->run((adjustedDelta > 0) ? FORWARD : BACKWARD);

    const unsigned long t0 = millis();
    while (true) {
        const long err = target - readLaunchCount();
        if (labs(err) <= LAUNCH_POSITION_TOLERANCE) {
            break;
        }
        if (millis() - t0 > LAUNCH_TIMEOUT_MS) {
            Serial.println(F("[WARN] Custom rotate timeout."));
            break;
        }
        delay(1);
    }

    // 到位后刹车停转
    launchMotorDirection = 0;
    launchMotor->setSpeed(0);
    launchMotor->run(RELEASE);
    
    Serial.println(F("ROTATE Complete."));
}
