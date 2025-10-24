#include <Arduino.h>
#define LED_PIN 8  // LED intégrée du SuperMini

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    delay(2000);  // Attend 2 secondes
    Serial.println("\n\n=== DEMARRAGE ===");
    Serial.println("Si vous voyez ceci, le Serial fonctionne!");

}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED ON - Message de test");
    delay(300);

    digitalWrite(LED_PIN, LOW);
    Serial.println("LED OFF - Message de test");
    delay(300);
}


