#include <Servo.h>

const int PIN_SERVO = 10;
const int PIN_TRIG  = 12;
const int PIN_ECHO  = 13;

const int ANG_CLOSED = 0;     // 차단기 내림
const int ANG_OPEN   = 90;    // 차단기 올림
const unsigned long MOVE_MS = 1500;   // 이동 시간(부드러움 조절)

// 거리 임계값(mm): 임계 이하=열림, 이상=닫힘
const float TH_ON_MM  = 300.0;   // 30cm 이내면 OPEN
const float TH_OFF_MM = 500.0;   // 50cm 이상이면 CLOSE (히스테리시스)

Servo sv;

// ---- Motion State (non-blocking) ----
struct Motion {
  bool active = false;
  unsigned long t0 = 0;
  unsigned long T  = 0;
  float a0 = 0.0f, a1 = 0.0f;
} mv;

// quintic minimum-jerk (0..1 -> 0..1), v=a=0 at ends
static inline float ease_minjerk(float t) {
  // clamp
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  float t2 = t*t, t3 = t2*t, t4 = t3*t, t5 = t4*t;
  return 10.0f*t3 - 15.0f*t4 + 6.0f*t5;
}

void startMove(float fromDeg, float toDeg, unsigned long durationMs) {
  mv.active = true;
  mv.t0 = millis();
  mv.T  = durationMs;
  mv.a0 = fromDeg;
  mv.a1 = toDeg;
}

void updateMove() {
  if (!mv.active) return;
  unsigned long now = millis();
  float t = (mv.T == 0) ? 1.0f : (float)(now - mv.t0) / (float)mv.T;
  if (t >= 1.0f) t = 1.0f;

  float u = ease_minjerk(t);
  float ang = mv.a0 + (mv.a1 - mv.a0) * u;

  // 서보 해상도를 높이고 기계적 미세 떨림을 줄이고 싶다면 아래 주석 해제:
  // long us = map((long)ang, 0, 180, 1000, 2000);
  // sv.writeMicroseconds(us);
  sv.write((int)(ang + 0.5f));

  if (t >= 1.0f) mv.active = false;
}

long read_distance_us() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  // timeout 50ms (50000us) for robustness
  return pulseIn(PIN_ECHO, HIGH, 50000UL);
}

float distance_mm() {
  // sound speed ~ 346 m/s => 0.346 mm/us; round trip => *0.5
  long us = read_distance_us();
  if (us == 0) return 9999.0f; // timeout -> very far
  return us * 0.346f * 0.5f;
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  sv.attach(PIN_SERVO);
  sv.write(ANG_CLOSED);
}

void loop() {
  updateMove();

  static bool opened = false;
  static unsigned long lastSense = 0;
  if (millis() - lastSense >= 60) { // 60ms 샘플링
    lastSense = millis();
    float d = distance_mm();

    // 히스테리시스: 깜빡임 방지
    if (!opened && d <= TH_ON_MM && !mv.active) {
      startMove(ANG_CLOSED, ANG_OPEN, MOVE_MS);
      opened = true;
    } else if (opened && d >= TH_OFF_MM && !mv.active) {
      startMove(ANG_OPEN, ANG_CLOSED, MOVE_MS);
      opened = false;
    }
  }
}
