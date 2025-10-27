// ========================
//  Ultrasonic + Servo Demo
//  (조건: 슬라이드 2장 그대로 반영)
// ========================

#include <Servo.h>

// -------- 핀 배치 --------
#define PIN_LED    9     // LED (active-low)
#define PIN_TRIG   12    // HC-SR04 TRIG
#define PIN_ECHO   13    // HC-SR04 ECHO
#define PIN_SERVO  10    // Servo PWM

// -------- 초음파/샘플링 파라미터 --------
#define SND_VEL        346.0     // m/s (약 24°C)
#define INTERVAL       25        // ms (샘플링 주기)
#define PULSE_DURATION 10        // us (TRIG 펄스 폭)

// 거리 범위 (mm) : 18cm~36cm
#define _DIST_MIN 180.0
#define _DIST_MAX 360.0

// pulseIn 변환/타임아웃
#define TIMEOUT ((INTERVAL / 2) * 1000.0)     // us
#define SCALE   (0.001 * 0.5 * SND_VEL)       // us * SCALE -> mm

// -------- EMA 필터 --------
// ※ α 값은 실험으로 선택 (0.1~0.6 권장)
#define _EMA_ALPHA 0.30

// -------- 서보 설정 --------
// attach 시 펄스 범위 명시(대부분의 SG90/MG90S 호환)
#define SERVO_MIN_US  500   // ≈ 0°
#define SERVO_MAX_US  2400  // ≈ 180°

Servo myservo;

// -------- 전역 변수 --------
float dist_ema = _DIST_MIN;          // EMA 초기값
unsigned long last_ms = 0;           // 샘플링 타임스탬프

// -------- 프로토타입 --------
static float measure_mm(int TRIG, int ECHO);  // 초음파 거리(mm)

// ========================
//            setup
// ========================
void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);  // active-low: 기본 OFF

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  myservo.attach(PIN_SERVO, SERVO_MIN_US, SERVO_MAX_US);
  myservo.write(90);  // 시작 각도

  Serial.begin(57600);

  last_ms = millis();
}

// ========================
//            loop
// ========================
void loop() {
  // 고정 주기 샘플링
  if (millis() - last_ms < INTERVAL) return;
  last_ms += INTERVAL;

  // 1) 초음파 원시 측정
  float dist_raw = measure_mm(PIN_TRIG, PIN_ECHO); // mm

  // 2) 범위 필터 (포화) + 실패값 방지
  float dist_filt;
  if (dist_raw == 0.0f) {              // timeout 등
    dist_filt = _DIST_MAX;             // 상한으로 포화
  } else if (dist_raw < _DIST_MIN) {
    dist_filt = _DIST_MIN;
  } else if (dist_raw > _DIST_MAX) {
    dist_filt = _DIST_MAX;
  } else {
    dist_filt = dist_raw;
  }

  // 3) EMA 적용 : ema = α*new + (1-α)*old
  dist_ema = _EMA_ALPHA * dist_filt + (1.0f - _EMA_ALPHA) * dist_ema;

  // 4) LED : 18~36cm 범위 내에서 점등 (active-low)
  if (dist_filt >= _DIST_MIN && dist_filt <= _DIST_MAX) {
    digitalWrite(PIN_LED, LOW);   // ON
  } else {
    digitalWrite(PIN_LED, HIGH);  // OFF
  }

  // 5) 거리→각도 연속 매핑
  //    18cm 이하: 0°, 18~36cm: 선형 0→180°, 36cm 이상: 180°
  float x = dist_ema;
  if (x < _DIST_MIN) x = _DIST_MIN;
  if (x > _DIST_MAX) x = _DIST_MAX;

  float p = (x - _DIST_MIN) / (_DIST_MAX - _DIST_MIN); // 0..1
  int angle = (int)(p * 180.0f + 0.5f);                // 0..180°
  myservo.write(angle);                                 // 각도 기반 제어

  // 6) 플로터 친화 시리얼 출력 (슬라이드 형식)
  //    min(..., _DIST_MAX + 100)로 초기 과도/튀는 값으로 y축이 넓어지는 것 방지
  Serial.print("Min:");    Serial.print(_DIST_MIN);
  Serial.print(",dist:");  Serial.print(min(dist_raw, _DIST_MAX + 100));
  Serial.print(",ema:");   Serial.print(min(dist_ema, _DIST_MAX + 100));
  Serial.print(",Servo:"); Serial.print(myservo.read()); // 라이브러리 기준 각도
  Serial.print(",Max:");   Serial.print(_DIST_MAX);
  Serial.println("");
}

// ========================
//      초음파 측정 (mm)
// ========================
static float measure_mm(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  // echo 펄스 폭(us) -> 거리(mm)
  unsigned long t = pulseIn(ECHO, HIGH, (unsigned long)TIMEOUT);
  if (t == 0) return 0.0f;        // timeout
  return t * SCALE;               // us * (0.001 * 0.5 * v) = mm
}
