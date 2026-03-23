// N20 按钮控制测试程序
// 1) 按试射按钮(D11) -> N20 正向转 360 度
// 2) 按摇杆按钮(D5)  -> N20 反向转 360 度

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <Servo.h>
#include "Config.h"

constexpr uint8_t PIN_ENCODER_A = PIN_LAUNCH_ENCA;

constexpr uint8_t MOTOR_PORT = 1;
constexpr uint8_t MOTOR_SPEED = 180;

// 每圈计数，请按你的电机微调：常见 3278(11线) 或 2086(7线)
constexpr long COUNTS_PER_360 = 3278;
constexpr long POSITION_TOLERANCE = 4;
constexpr unsigned long DEBOUNCE_MS = 40;
constexpr unsigned long MOVE_TIMEOUT_MS = 6000;
constexpr int JOY_CENTER = 512;
constexpr int JOY_DEADZONE = 100;
constexpr int SERVO_STEP = 1;

Adafruit_MotorShield shield;
Adafruit_DCMotor *motor = nullptr;
Servo servoYaw;
Servo servoPitch;

volatile long encoderCount = 0;
volatile int8_t motorDirection = 0;

int yawAngle = 90;
int pitchAngle = 90;

bool lastFireBtn = HIGH;
bool lastJoyBtn = HIGH;
unsigned long lastFireTriggerMs = 0;
unsigned long lastJoyTriggerMs = 0;

void ISR_EncoderA() {
    encoderCount += motorDirection;
}

long readCount() {
    noInterrupts();
    long v = encoderCount;
    interrupts();
    return v;
}

void releaseMotor() {
    motorDirection = 0;
    motor->setSpeed(0);
    motor->run(RELEASE);
}

void rotateRelativeCounts(long delta) {
    const long start = readCount();
    const long target = start + delta;

    motorDirection = (delta > 0) ? 1 : -1;
    motor->setSpeed(MOTOR_SPEED);
    motor->run((delta > 0) ? FORWARD : BACKWARD);

    const unsigned long t0 = millis();
    while (true) {
        const long err = target - readCount();
        if (labs(err) <= POSITION_TOLERANCE) {
            break;
        }
        if (millis() - t0 > MOVE_TIMEOUT_MS) {
            Serial.println(F("[WARN] Move timeout, stop motor."));
            break;
        }
        delay(1);
    }

    releaseMotor();
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_TEST_FIRE_BTN, INPUT_PULLUP);
    pinMode(PIN_JOY_BTN, INPUT_PULLUP);
    pinMode(PIN_ENCODER_A, INPUT_PULLUP);
    delay(20);

    lastFireBtn = digitalRead(PIN_TEST_FIRE_BTN);
    lastJoyBtn = digitalRead(PIN_JOY_BTN);

    // 开机诊断：打印各按钮引脚实际电平
    Serial.print(F("[DIAG] PIN_TEST_FIRE_BTN(D"));
    Serial.print(PIN_TEST_FIRE_BTN);
    Serial.print(F(") = "));
    Serial.println(digitalRead(PIN_TEST_FIRE_BTN) ? F("HIGH") : F("LOW"));
    Serial.print(F("[DIAG] PIN_JOY_BTN(D"));
    Serial.print(PIN_JOY_BTN);
    Serial.print(F(") = "));
    Serial.println(digitalRead(PIN_JOY_BTN) ? F("HIGH") : F("LOW"));

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), ISR_EncoderA, RISING);

    if (!shield.begin()) {
        Serial.println(F("Motor Shield init failed."));
        while (true) {
            delay(100);
        }
    }

    motor = shield.getMotor(MOTOR_PORT);
    releaseMotor();

    servoYaw.attach(PIN_SERVO_YAW);
    servoPitch.attach(PIN_SERVO_PITCH);
    servoYaw.write(yawAngle);
    servoPitch.write(pitchAngle);

    Serial.println(F("Ready: D11=Forward360, D5=Backward360, Joystick(A0/A1)=Servo"));
}

void loop() {
    // --- 摇杆实时控制双舵机 ---
    const int joyX = analogRead(PIN_JOY_X);
    const int joyY = analogRead(PIN_JOY_Y);

    if (joyX < JOY_CENTER - JOY_DEADZONE) {
        yawAngle += SERVO_STEP;
    } else if (joyX > JOY_CENTER + JOY_DEADZONE) {
        yawAngle -= SERVO_STEP;
    }

    if (joyY < JOY_CENTER - JOY_DEADZONE) {
        pitchAngle += SERVO_STEP;
    } else if (joyY > JOY_CENTER + JOY_DEADZONE) {
        pitchAngle -= SERVO_STEP;
    }

    yawAngle = constrain(yawAngle, 0, 180);
    pitchAngle = constrain(pitchAngle, 0, 180);
    servoYaw.write(yawAngle);
    servoPitch.write(pitchAngle);

    // --- 按钮触发N20单圈动作 ---
    const bool fireNow = digitalRead(PIN_TEST_FIRE_BTN);
    const bool joyNow = digitalRead(PIN_JOY_BTN);

    if (fireNow != lastFireBtn) {
        Serial.print(F("[BTN] TEST_FIRE = "));
        Serial.println(fireNow ? F("HIGH") : F("LOW"));
    }

    if (joyNow != lastJoyBtn) {
        Serial.print(F("[BTN] JOY_BTN = "));
        Serial.println(joyNow ? F("HIGH") : F("LOW"));
    }

    if (lastFireBtn == HIGH && fireNow == LOW && (millis() - lastFireTriggerMs) > DEBOUNCE_MS) {
        lastFireTriggerMs = millis();
        Serial.println(F("Forward 360"));
        rotateRelativeCounts(+COUNTS_PER_360);
    }

    if (lastJoyBtn == HIGH && joyNow == LOW && (millis() - lastJoyTriggerMs) > DEBOUNCE_MS) {
        lastJoyTriggerMs = millis();
        Serial.println(F("Backward 360"));
        rotateRelativeCounts(-COUNTS_PER_360);
    }

    lastFireBtn = fireNow;
    lastJoyBtn = joyNow;

    delay(15);
}
