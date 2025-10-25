#include <Arduino.h>

#define LED_PIN 8  // LED intégrée du SuperMini


void setup() {

  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(5000);
  Serial.println("Start clock...");


}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(300);

    digitalWrite(LED_PIN, LOW);
    delay(300);
    Serial.println("loop clock");
}
