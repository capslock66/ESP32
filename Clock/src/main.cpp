#include <Arduino.h>

#define STEP_PULSE_US   5  // ms

#define LED_PIN         8  // LED intégrée du SuperMini
#define Start_PIN       0  // Pin de démarrage de l'horloge

// Motor
#define Dir             10
#define Step            20
#define Enabled         21

int readCounter = 0;
int startClock  = 0;
bool motorRunning = false;
void readSensors()
{
    readCounter++;
    //Serial.println("Counter: " + String(readCounter) );

    int oldStartClock = startClock;
    startClock = digitalRead(Start_PIN);       // 0

    if (oldStartClock != startClock)
    {
        //Serial.println("StartClock changed from " + String(oldStartClock) + " to " + String(startClock));
        if (startClock == HIGH) {
            if (motorRunning)
            {
                Serial.println("Motor Stopped");
                motorRunning = false;
            }
            else
            {
                Serial.println("Motor Started ");
                motorRunning = true;
            }
        }
    }
}

void setup()
{

    pinMode(LED_PIN, OUTPUT);   // 8
    pinMode(Start_PIN , INPUT_PULLDOWN);   // 0

    pinMode(Dir, OUTPUT);      // 10
    pinMode(Enabled, OUTPUT);  // 21
    pinMode(Step, OUTPUT);     // 20

    digitalWrite(Dir, HIGH);     // Sens horaire
    digitalWrite(Enabled, HIGH); // Desactivation des drivers (HIGH = désactivé)

    Serial.begin(115200);
    delay(5000);
    Serial.println("Clock waiting");

}

void stepperMove()
{
    digitalWrite(Dir, HIGH); // LOW: sens horaire, HIGH: sens anti-horaire (mais horaire de l'autre coté du moteur)
    unsigned long startTime = millis();
    const unsigned long DURATION = 70000; // 70 seconds
    int lastPrintSeconds = millis();
    int stepCount = 0;
    while (true)
    {
        readSensors();
        unsigned long currentTime = millis() ;

        if (millis() >= lastPrintSeconds + 1000)
        {
            stepCount++;
            Serial.println("Delay: " + String(stepCount) + "/" + String(DURATION/1000) + " seconds");
            lastPrintSeconds = currentTime;
        }

        if (!motorRunning || (currentTime - startTime >= DURATION))
            break;

        if (!motorRunning)
            break;

        digitalWrite(Step, HIGH);
        delayMicroseconds(STEP_PULSE_US);    // 5 us
        digitalWrite(Step, LOW);
        delayMicroseconds(10000);            // 10000 us = 10 ms
    }
}

void loop()
{
    readSensors();
    if (motorRunning)
    {
        digitalWrite(Enabled, LOW); // Activation des drivers (LOW = activé)
        stepperMove();
        motorRunning = false;
        digitalWrite(Enabled, HIGH); // Desactivation des drivers (HIGH = désactivé)
        Serial.println("End of stepper Move");
    } else {
        digitalWrite(LED_PIN, HIGH);
        delay(100); // LED on for 100 ms
        readSensors();
        digitalWrite(LED_PIN, LOW);
        delay(100); // LED off for 100 ms
    }
    //Serial.println("loop clock");
}
