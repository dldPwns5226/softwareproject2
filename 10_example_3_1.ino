#include <Servo.h>

#define PIN_SERVO 10
#define PIN_TRIG 12
#define PIN_ECHO 13

const int ANGLE_CLOSED = 30;
const int ANGLE_OPEN   = 120;
unsigned long MOVING_TIME = 3000;

const int OPEN_MM_ON  = 300;
const int OPEN_MM_OFF = 400;

const unsigned long TIMEOUT_US = 30000UL;
const float SND_VEL  = 346.0f;
const float SCALE_MM = 0.001f * 0.5f * SND_VEL;

Servo servo;
bool gateOpen = false;
bool moving = false;
unsigned long moveStartTime;
int startAngle = ANGLE_CLOSED;
int stopAngle  = ANGLE_OPEN;

float measure_mm() {
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, TIMEOUT_US);
  if (us == 0) return 0.0f;
  return us * SCALE_MM;
}

// sigmoid easing
float ease_sigmoid(float u) {
  if (u <= 0) return 0; if (u >= 1) return 1;
  const float k = 10.0f;
  float x = (u - 0.5f) * k * 2.0f;
  return 1.0f / (1.0f + expf(-x));
}

void setup() {
  servo.attach(PIN_SERVO);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  servo.write(ANGLE_CLOSED);
  delay(500);
}

void loop() {
  float d = measure_mm();
  if (!moving) {
    if (!gateOpen && d > 0 && d <= OPEN_MM_ON) {
      startAngle = servo.read(); stopAngle = ANGLE_OPEN;
      moveStartTime = millis(); moving = true; gateOpen = true;
    } else if (gateOpen && (d == 0 || d >= OPEN_MM_OFF)) {
      startAngle = servo.read(); stopAngle = ANGLE_CLOSED;
      moveStartTime = millis(); moving = true; gateOpen = false;
    }
  }

  if (moving) {
    unsigned long progress = millis() - moveStartTime;
    if (progress <= MOVING_TIME) {
      float u = (float)progress / MOVING_TIME;
      float w = ease_sigmoid(u);
      int angle = startAngle + (stopAngle - startAngle) * w;
      servo.write(angle);
    } else {
      servo.write(stopAngle);
      moving = false;
    }
  }
}
