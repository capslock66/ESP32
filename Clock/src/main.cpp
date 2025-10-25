#include <Arduino.h>

#define LED_PIN 8  // LED intégrée du SuperMini

#define StartClock       0
#define StopClock        1

// Motor
#define Dir           10
#define Step          20
#define Enabled       21

int readCounter = 0;
int startClock  = 0;
int stopClock   = 0;

void readSensors()
{
    readCounter++;

    int oldStartClock = startClock;
    int oldStopClock  = stopClock;

    startClock      = digitalRead(StartClock);       // 0
    stopClock       = digitalRead(StopClock);        // 1

    if (oldStartClock != startClock)
        Serial.println("StartClock changed from " + String(oldStartClock) + " to " + String(startClock));
    if (oldStopClock != stopClock)
        Serial.println("StopClock changed from " + String(oldStopClock) + " to " + String(stopClock));

}


void setup()
{

    pinMode(LED_PIN, OUTPUT);   // 8

    pinMode(StartClock       , INPUT_PULLDOWN);   // 0
    pinMode(StopClock        , INPUT_PULLDOWN);   // 1

    pinMode(Dir, OUTPUT);      // 10
    pinMode(Step, OUTPUT);     // 20
    pinMode(Enabled, OUTPUT);  // 21

    digitalWrite(Enabled, HIGH); // Desactivation des drivers (HIGH = désactivé)

    Serial.begin(115200);
    delay(5000);
    Serial.println("Start clock...");


}

void loop()
{

    readSensors();

    digitalWrite(LED_PIN, HIGH);
    delay(300);

    digitalWrite(LED_PIN, LOW);
    delay(300);
    //Serial.println("loop clock");
}
