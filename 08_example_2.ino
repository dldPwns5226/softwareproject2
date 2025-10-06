#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 13

#define SND_VEL 346.0
#define INTERVAL 25
#define PULSE_DURATION 10
#define _DIST_MIN 100.0
#define _DIST_MAX 300.0

#define TIMEOUT ((INTERVAL / 2) * 1000.0)
#define SCALE (0.001 * 0.5 * SND_VEL)

unsigned long last_sampling_time;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  Serial.begin(57600);
  last_sampling_time = millis();
}

void loop() {
  if (millis() - last_sampling_time < INTERVAL)
    return;

  float distance = USS_measure(PIN_TRIG, PIN_ECHO);

  int pwm;
  if (distance == 0.0 || distance <= _DIST_MIN || distance >= _DIST_MAX) {
    pwm = 255;
  } else if (distance <= 200.0) {
    pwm = (int)(255.0 - (distance - 100.0) * (255.0 / 100.0));
  } else {
    pwm = (int)((distance - 200.0) * (255.0 / 100.0));
  }
  if (pwm < 0) pwm = 0;
  if (pwm > 255) pwm = 255;
  analogWrite(PIN_LED, pwm);

  Serial.print("Min:");        Serial.print(_DIST_MIN);
  Serial.print(",distance:");  Serial.print(distance);
  Serial.print(",Max:");       Serial.print(_DIST_MAX);
  Serial.print(",pwm:");       Serial.println(pwm);

  last_sampling_time += INTERVAL;
}

float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE;
}
