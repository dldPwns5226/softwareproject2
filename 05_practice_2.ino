#define LED_PIN 7
const int ON  = LOW;
const int OFF = HIGH;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, OFF);
}

void loop() {
  digitalWrite(LED_PIN, ON);   delay(1000);
  for (int i=0;i<5;i++) {
    digitalWrite(LED_PIN, ON); delay(100);
    digitalWrite(LED_PIN, OFF);delay(100);
  }
  digitalWrite(LED_PIN, OFF);
  while (1) {;}
}
