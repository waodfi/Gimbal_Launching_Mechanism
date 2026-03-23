#include "LauncherModule.h"
#include <Adafruit_MotorShield.h>

// I2C地址默认0x60
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_DCMotor *launchMotor = AFMS.getMotor(1); // M1: 发射电机
Adafruit_DCMotor *feedMotor = AFMS.getMotor(2);   // M2: 供弹电机

// 需要给硬中断使用的编码器计数器
volatile long feedEncoderCount = 0;
volatile long launchEncoderCount = 0;

// ====== 编码器与电机运行参数微调区 ======
// 你的 N20 电机减速比为 298。
// 假设尾部编码器为 7 线，则单圈总脉冲 = 298 * 7 = 2086 （若是 11 线则是 3278）
#define LAUNCH_PULSES_PER_REV 2086 // 发射机构拉拉杆转一圈所需脉冲
#define FEED_PULSES_PER_SLOT  347  // 供弹机构转60度所需脉冲 (2086 / 6 ≈ 347)

// 发射与供弹的电机转速 (0-255)
#define LAUNCH_SPEED 255
#define FEED_SPEED 150

// 中断服务函数 (只用A相单向计数，针对单向发/供弹机制最稳定)
void ISR_FeedEncoderA() {
    feedEncoderCount++; 
}

void ISR_LaunchEncoderA() {
    launchEncoderCount++;
}

void Launcher_Init() {
    if (!AFMS.begin()) {
        Serial.println("Could not find Motor Shield. Check wiring.");
        while (1); // 找不到板子就死循环
    }
    
    // 初始化编码器引脚
    pinMode(PIN_FEED_ENCA, INPUT_PULLUP);
    pinMode(PIN_FEED_ENCB, INPUT_PULLUP);
    pinMode(PIN_LAUNCH_ENCA, INPUT_PULLUP);
    pinMode(PIN_LAUNCH_ENCB, INPUT_PULLUP);

    // 绑定外部中断 (D2=中断0, D3=中断1)
    attachInterrupt(digitalPinToInterrupt(PIN_LAUNCH_ENCA), ISR_LaunchEncoderA, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_FEED_ENCA), ISR_FeedEncoderA, RISING);

    // 初始状态停转
    launchMotor->setSpeed(0);
    launchMotor->run(RELEASE);
    feedMotor->setSpeed(0);
    feedMotor->run(RELEASE);

    Serial.println("Launcher Module Initialized.");
}

void Launcher_Fire() {
    Serial.println("Action: FIRE!");
    launchEncoderCount = 0; // 计步清零
    
    launchMotor->setSpeed(LAUNCH_SPEED);
    launchMotor->run(FORWARD); // 设定发射方向 (如果反了这里改成 BACKWARD)

    // 阻塞等待电机旋转到达指定圈数
    while (launchEncoderCount < LAUNCH_PULSES_PER_REV) {
        // 如果需要可以增加超时保护防止堵转
        delay(1); 
    }

    // 到位后刹车停转
    launchMotor->setSpeed(0);
    launchMotor->run(RELEASE);
    Serial.println("FIRE Complete.");
}

void Launcher_FeedBullet() {
    Serial.println("Action: FEED BULLET!");
    feedEncoderCount = 0; // 计步清零

    feedMotor->setSpeed(FEED_SPEED);
    feedMotor->run(FORWARD); // 设定供弹方向

    // 阻塞等待电机旋转 60 度
    while (feedEncoderCount < FEED_PULSES_PER_SLOT) {
        delay(1);
    }

    // 到位后停转
    feedMotor->setSpeed(0);
    feedMotor->run(RELEASE);
    Serial.println("FEED Complete.");
}
