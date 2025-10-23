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
#define MaxOpenedDoorA  9
#define MaxClosedDoorA 10

// Motor A
#define StepA          11
#define DirA           12
#define EnabledA       13

// door 2 detection
#define MaxOpenedDoorB 14
#define MaxClosedDoorB 15

// Motor B
#define StepB          16
#define DirB           17
#define EnabledB       18


#define STEP_PULSE_US 5

int loopCounter    = 0;
int readCounter    = 0;
int openDoors      = 0;
int closeDoors     = 0;
int doorsAction    = 0;
int maxOpenedDoorA = 0;
int maxClosedDoorA = 0;
int maxOpenedDoorB = 0;
int maxClosedDoorB = 0;
bool doStepMotorA  = false;
bool doStepMotorB  = false;
long loopStep      = 0;

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
    pinMode(StepA, OUTPUT);     // 11
    pinMode(DirA, OUTPUT);      // 12
    pinMode(EnabledA, OUTPUT);  // 13

    pinMode(StepB, OUTPUT);     // 16
    pinMode(DirB, OUTPUT);      // 17
    pinMode(EnabledB, OUTPUT);  // 18

    pinMode(OpenDoors       , INPUT_PULLDOWN);   //5
    pinMode(CloseDoors      , INPUT_PULLDOWN);   //6
    pinMode(MaxOpenedDoorA  , INPUT_PULLDOWN);   //9
    pinMode(MaxClosedDoorA  , INPUT_PULLDOWN);   //10
    pinMode(MaxOpenedDoorB  , INPUT_PULLDOWN);   //14
    pinMode(MaxClosedDoorB  , INPUT_PULLDOWN);   //15
    delay(100);

    digitalWrite(EnabledA, HIGH); // Desactivation des drivers (HIGH = désactivé)
    digitalWrite(EnabledB, HIGH);
}

void readSensors()
{
    readCounter++;
    maxOpenedDoorB = digitalRead(MaxOpenedDoorB);  // 14
    maxClosedDoorB = digitalRead(MaxClosedDoorB);  // 15
    openDoors      = digitalRead(OpenDoors);       // 5
    closeDoors     = digitalRead(CloseDoors);      // 6
    maxClosedDoorA = digitalRead(MaxClosedDoorA);  // 10
    maxOpenedDoorA = digitalRead(MaxOpenedDoorA);  // 9
}

void printVars()
{
    char bufStep[6];
    snprintf(bufStep, sizeof(bufStep), "%5ld", loopStep);

    Serial.println(
        String(readCounter)
        + "/"   + bufStep
        + "------"
        + ", MaxOB: "  + String(maxOpenedDoorB)
        + ", MaxCB: "  + String(maxClosedDoorB)
        + "------"
        + ", Open: "   + String(openDoors)
        + ", Close: "  + String(closeDoors)
        + "------"
        + ", MaxCA: "  + String(maxClosedDoorA)
        + ", MaxOA: "  + String(maxOpenedDoorA)
        + "------"
        + ", MotorA: " + String(doStepMotorA)
        + ", MotorB: " + String(doStepMotorB)
    );
}

void stepperMove(
    float revs_per_sec_max,     // 5 tours/s
    unsigned int microstepDen,  // 2
    long nbTours)               // 25
{


    if (doorsAction == 1)
    {
        digitalWrite(DirA, HIGH); // Direction: sens horaire
        digitalWrite(DirB, HIGH);
    } else if (doorsAction == 2) {  // close doors
        digitalWrite(DirA, LOW); // Direction: sens antihoraire
        digitalWrite(DirB, LOW);
    }

    const unsigned int fullStepsPerRev = 200;
    long totalSteps = nbTours * fullStepsPerRev * microstepDen;

    // vitesse de départ et vitesse max en pas/s
    float startStepsPerSec = 200.0;   // démarrage lent (~1 tr/s en 1/8 microstep)
    float maxStepsPerSec   = revs_per_sec_max * fullStepsPerRev * microstepDen;

    // rampes = % du mouvement total
    // long rampSteps = 200; // totalSteps / 10; // 4 = 25% accél, 25% décél, 8 = 12.5%, 10 = 10%
    // if (rampSteps < 10)
    //     rampSteps = 10; // sécurité

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);

    tft.setCursor(10, 10);
    tft.println(String(loopCounter++));

    tft.setCursor(10, 25);
    tft.println("doorsAction:" + String(doorsAction));

    tft.setCursor(10, 40);
    tft.println("totalSteps:" + String(totalSteps));

    for (loopStep = 0; loopStep < totalSteps; loopStep++)
    {
        // calcul de la vitesse courante en pas/s
        float currentStepsPerSec = maxStepsPerSec;

        // période par pas
        float period_us = 1e6 / currentStepsPerSec;
        unsigned long delayLow = (unsigned long)(period_us - STEP_PULSE_US);    // 5

        doStepMotorA=false;
        doStepMotorB=false;

        int oldMaxOpenedDoorB = maxOpenedDoorB;
        int oldMaxClosedDoorB = maxClosedDoorB;
        int oldMaxClosedDoorA = maxClosedDoorA;
        int oldMaxOpenedDoorA = maxOpenedDoorA;

        readSensors();

        if (oldMaxOpenedDoorB != maxOpenedDoorB)
            Serial.println("MaxOpenedDoorB changed from " + String(oldMaxOpenedDoorB) + " to " + String(maxOpenedDoorB));
        if (oldMaxClosedDoorB != maxClosedDoorB)
            Serial.println("MaxClosedDoorB changed from " + String(oldMaxClosedDoorB) + " to " + String(maxClosedDoorB));
        if (oldMaxClosedDoorA != maxClosedDoorA)
            Serial.println("MaxClosedDoorA changed from " + String(oldMaxClosedDoorA) + " to " + String(maxClosedDoorA));
        if (oldMaxOpenedDoorA != maxOpenedDoorA)
            Serial.println("MaxOpenedDoorA changed from " + String(oldMaxOpenedDoorA) + " to " + String(maxOpenedDoorA));

        if (
            ((doorsAction == 1 /*Open the door */ && maxOpenedDoorA == LOW) /*&& openDoors  == HIGH */)  ||
            ((doorsAction == 2 /*Close the door*/ && maxClosedDoorA == LOW) /*&& closeDoors == HIGH */)
           )
        {
            doStepMotorA = true;
        }
        if (
            ((doorsAction == 1 /*Open the door */ && maxOpenedDoorB == LOW) /*&& openDoors  == HIGH */)  ||
            ((doorsAction == 2 /*Close the door*/ && maxClosedDoorB == LOW) /*&& closeDoors == HIGH */)
           )
        {
           doStepMotorB = true;
        }

        // if no motor to step, exit loop
        if (doStepMotorA == false && doStepMotorB == false)
        {
            Serial.println("Both Max detected. Break");
            break;
        }

        //printVars();

        if (doStepMotorA) digitalWrite(StepA, HIGH);
        if (doStepMotorB) digitalWrite(StepB, HIGH);

        if (doStepMotorA || doStepMotorB)
            delayMicroseconds(STEP_PULSE_US);   // 5 ms

        if (doStepMotorA) digitalWrite(StepA, LOW);
        if (doStepMotorB) digitalWrite(StepB, LOW);

        if ((doStepMotorA || doStepMotorB) && delayLow > 0)
            delayMicroseconds(delayLow);        // 5 ms

    }
}

void loop()
{
    readSensors();
    //printVars();

    doorsAction = 0;
    if (openDoors == HIGH)
        doorsAction = 1;
    if (closeDoors == HIGH)
        doorsAction = 2;

    if (doorsAction != 0)
    {
        Serial.println("Start of stepper Move");
        digitalWrite(EnabledA, LOW); // Activation des drivers (LOW = activé)
        digitalWrite(EnabledB, LOW);
        stepperMove(
            5.0,    // revs_per_sec_max : vitesse max = 5 tours/s
            2,      // microstepDen = 1/2
            25      // nbTours
        );
        doStepMotorA=false;
        doStepMotorB=false;
        digitalWrite(EnabledA, HIGH); // Desactivation des drivers (HIGH = désactivé)
        digitalWrite(EnabledB, HIGH);
        Serial.println("End of stepper Move");
        loopStep = 0;
    }
    //delay(500);
}
