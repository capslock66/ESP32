#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// --- Broches correctes Feather ESP32‑S3 TFT ---
#define TFT_CS 7
#define TFT_DC 39
#define TFT_RST 40
#define TFT_BL 45
#define TFT_SCK 36
#define TFT_MOSI 35

SPIClass spi = SPIClass(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST);

// OpenDoor (both)
#define OpenDoors       5
#define CloseDoors      6

// door 1 detection
#define MaxOpenedDoor1  9
#define MaxClosedDoor1 10

// Motor 1
#define Step1          11
#define Dir1           12
#define Enabled1       13

// door 2 detection
#define MaxOpenedDoor2 14
#define MaxClosedDoor2 15

// Motor 2
#define Step2          16
#define Dir2           17
#define Enabled2       18


#define STEP_PULSE_US 5

int loopCounter = 0;

void setup()
{
    Serial.begin(115200);
    // while (!Serial)
    //     ;

    // Serial.println("Init écran...");

    // Allume le rétroéclairage
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // SPI sur les bons pins (MISO non utilisé)
    spi.begin(TFT_SCK, -1, TFT_MOSI);

    // Init écran
    tft.init(135, 240); // H x L
    tft.setRotation(3); // paysage
    tft.fillScreen(ST77XX_BLACK);

    // Test d'affichage on 12 lines
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1); // 1 Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
    tft.setCursor(10, 0);
    tft.println("line 1: Feather ESP32-S3");

    tft.setTextColor(ST77XX_WHITE);
    for (int i = 0; i < 11; i++)
    {
        tft.setCursor(10, 10 + i * 10);
        tft.println("Line " + String(i + 2));
    }
    tft.setTextSize(2); // 1 Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
    delay(1000);

    // Serial.println("Ecran prêt !");

    // Initialisation Gpio
    pinMode(Step1, OUTPUT);
    pinMode(Dir1, OUTPUT);
    pinMode(Enabled1, OUTPUT);

    pinMode(Step2, OUTPUT);
    pinMode(Dir2, OUTPUT);
    pinMode(Enabled2, OUTPUT);

    pinMode(OpenDoors, INPUT_PULLDOWN);
    pinMode(CloseDoors, INPUT_PULLDOWN);

    pinMode(MaxOpenedDoor1, INPUT_PULLDOWN);
    pinMode(MaxClosedDoor1, INPUT_PULLDOWN);
    pinMode(MaxOpenedDoor2, INPUT_PULLDOWN);
    pinMode(MaxClosedDoor2, INPUT_PULLDOWN);
    delay(10);
}

// void stepperMoveRate(float revs_per_sec, unsigned int microstepDen, long stepsToDo )
// {
//     const unsigned int fullStepsPerRev = 200;
//     float steps_per_sec = revs_per_sec * fullStepsPerRev * microstepDen;
//     if (steps_per_sec <= 0.0)
//         return;

//     // période totale en µs par pas
//     float period_us = 1e6 / steps_per_sec;

//     unsigned long rest_us = (unsigned long)round(period_us) - STEP_PULSE_US;
//     if (rest_us < 0)
//         rest_us = 0;

//     long steps = stepsToDo;
//     while (steps != 0)
//     {
//         digitalWrite(Step1, HIGH);
//         delayMicroseconds(STEP_PULSE_US);
//         digitalWrite(Step1, LOW);
//         delayMicroseconds(rest_us);
//         if (steps > 0)
//             steps--;
//     }
// }

// void doStep (unsigned long delayLow, int8_t motor)
// {
//     if (motor == 1)
//     {
//         digitalWrite(Step1, HIGH);
//         delayMicroseconds(STEP_PULSE_US);
//         digitalWrite(Step1, LOW);
//     }
//     else if (motor == 2)
//     {
//         digitalWrite(Step2, HIGH);
//         delayMicroseconds(STEP_PULSE_US);
//         digitalWrite(Step2, LOW);
//     }

//     if (delayLow > 0)
//         delayMicroseconds(delayLow);
// }

// Déplacement avec accélération et décélération
void stepperMoveWithRamp(
    float revs_per_sec_max,     // 5 tours/s
    unsigned int microstepDen,  // 2
    long nbTours,               // 25
    int doorsAction)
{

    int maxDoorDetection1;
    int maxDoorDetection2;

    if (doorsAction == 1)
    {
        digitalWrite(Dir1, HIGH); // Direction: sens horaire
        digitalWrite(Dir2, HIGH);
        maxDoorDetection1 = MaxOpenedDoor1;
        maxDoorDetection2 = MaxOpenedDoor2;
    } else if (doorsAction == 2) {  // close doors
        digitalWrite(Dir1, LOW); // Direction: sens antihoraire
        digitalWrite(Dir2, LOW);
        maxDoorDetection1 = MaxClosedDoor1;
        maxDoorDetection2 = MaxClosedDoor2;
    } else {  // standby
        return;
    }

    const unsigned int fullStepsPerRev = 200;
    long totalSteps = nbTours * fullStepsPerRev * microstepDen;

    // vitesse de départ et vitesse max en pas/s
    float startStepsPerSec = 200.0;   // démarrage lent (~1 tr/s en 1/8 microstep)
    float maxStepsPerSec   = revs_per_sec_max * fullStepsPerRev * microstepDen;

    // rampes = % du mouvement total
    long rampSteps = 200; // totalSteps / 10; // 4 = 25% accél, 25% décél, 8 = 12.5%, 10 = 10%
    if (rampSteps < 10)
        rampSteps = 10; // sécurité

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);

        tft.setCursor(10, 10);
    tft.println(String(loopCounter++));

    tft.setCursor(10, 25);
    tft.println("totalSteps:" + String(totalSteps));

    tft.setCursor(10, 40);
    tft.println("rampSteps:" + String(rampSteps));

    for (long i = 0; i < totalSteps; i++)
    {
        // calcul de la vitesse courante en pas/s
        float currentStepsPerSec;

        if (i < rampSteps)
        {
            // phase accélération
            currentStepsPerSec = startStepsPerSec + (maxStepsPerSec - startStepsPerSec) * ((float)i / rampSteps);
        }
        else if (i > totalSteps - rampSteps)
        {
            // phase décélération
            long stepsFromEnd = totalSteps - i;
            currentStepsPerSec = startStepsPerSec + (maxStepsPerSec - startStepsPerSec) * ((float)stepsFromEnd / rampSteps);
        } else {
            // phase plateau
            currentStepsPerSec = maxStepsPerSec;
        }

        // période par pas
        float period_us = 1e6 / currentStepsPerSec;
        unsigned long delayLow = (unsigned long)(period_us - STEP_PULSE_US);

        bool doStepMotor1=false;
        bool doStepMotor2=false;

        if (
            ((doorsAction == 1 /*Open the door */ && digitalRead(MaxOpenedDoor1) == LOW) && digitalRead(OpenDoors ) == HIGH)  ||
            ((doorsAction == 2 /*Close the door*/ && digitalRead(MaxClosedDoor1) == LOW) && digitalRead(CloseDoors) == HIGH)
           )
           doStepMotor1 = true;

        if (
            ((doorsAction == 1 /*Open the door */ && digitalRead(MaxOpenedDoor2) == LOW) && digitalRead(OpenDoors) == HIGH)  ||
            ((doorsAction == 2 /*Close the door*/ && digitalRead(MaxClosedDoor2) == LOW) && digitalRead(CloseDoors) == HIGH)
           )
           doStepMotor2 = true;

        if (doStepMotor1) digitalWrite(Step1, HIGH);
        if (doStepMotor2) digitalWrite(Step2, HIGH);

        if (doStepMotor1 || doStepMotor2)
            delayMicroseconds(STEP_PULSE_US);

        if (doStepMotor1) digitalWrite(Step1, LOW);
        if (doStepMotor2) digitalWrite(Step2, LOW);

        if ((doStepMotor1 || doStepMotor2) && delayLow > 0)
            delayMicroseconds(delayLow);

    }
}

void loop()
{
    int openDoor = digitalRead(OpenDoors);
    int closeDoor = digitalRead(CloseDoors);

    //Serial.println("OpenDoor: " + String(openDoor) + ", CloseDoor: " + String(closeDoor));

    // tft.fillScreen(ST77XX_BLACK);
    // tft.setTextColor(ST77XX_GREEN);

    // tft.setCursor(10, 55);
    // tft.println(String(millis()/1000) + " sec");

    int maxDoorDetection1 = -1;
    int maxDoorDetection2 = -1;
    int doorsAction = 0;
    if (openDoor == HIGH)
        doorsAction = 1;
    if (closeDoor == HIGH)
        doorsAction = 2;

    if (doorsAction != 0)
    {
        digitalWrite(Enabled1, LOW); // Activation des drivers (LOW = activé)
        digitalWrite(Enabled2, LOW);
        stepperMoveWithRamp(
            5.0,    // revs_per_sec_max : vitesse max = 5 tours/s
            2,      // microstepDen = 1/2
            25,     // nbTours
            doorsAction
        );
    } else {
        digitalWrite(Enabled1, HIGH); // Desactivation des drivers (HIGH = désactivé)
        digitalWrite(Enabled2, HIGH);
    }

    delay(100);
}
