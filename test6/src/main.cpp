#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// --- Broches correctes Feather ESP32‑S3 TFT ---
#define TFT_CS           7
#define TFT_DC           39
#define TFT_RST          40
#define TFT_BL           45
#define TFT_SCK          36
#define TFT_MOSI         35

SPIClass spi = SPIClass(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST);

// OpenDoor (both)
#define OpenDoors        5
#define CloseDoors       6

// door 1 detection
#define MaxOpenedDoorA   9
#define MaxClosedDoorA   10

// Motor A
#define StepA            11
#define DirA             12
#define EnabledA         13

// door 2 detection
#define MaxOpenedDoorB   14
#define MaxClosedDoorB   15

// Motor B
#define StepB            16
#define DirB             17
#define EnabledB         18

// Delays
#define MaxWaitDoorMoved 10000   // 10000 * 500 micro seconds = 5 seconds open or close
#define StepPulse        5       // micro seconds
#define DelayLow         495     // micro seconds
#define DelayBeforeClose 10000   // micro seconds, 10 seconds


//int readCounter    = 0;
int loopCounter    = 0;
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
    delay(3000);

    Serial.println("Screen Init");

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
    Serial.println("init Gpios");

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

    Serial.println("waiting commands");
}

void readSensors()
{
    //readCounter++;

    bool changed = false;
    int oldOpenDoors      = openDoors;
    int oldCloseDoors     = closeDoors;
    int oldMaxOpenedDoorB = maxOpenedDoorB;
    int oldMaxClosedDoorB = maxClosedDoorB;
    int oldMaxClosedDoorA = maxClosedDoorA;
    int oldMaxOpenedDoorA = maxOpenedDoorA;

    openDoors      = digitalRead(OpenDoors);       // 5
    closeDoors     = digitalRead(CloseDoors);      // 6
    maxOpenedDoorB = digitalRead(MaxOpenedDoorB);  // 14
    maxClosedDoorB = digitalRead(MaxClosedDoorB);  // 15
    maxClosedDoorA = digitalRead(MaxClosedDoorA);  // 10
    maxOpenedDoorA = digitalRead(MaxOpenedDoorA);  // 9

    if (oldOpenDoors != openDoors)
    {
        Serial.println("OpenDoors changed from " + String(oldOpenDoors) + " to " + String(openDoors));
        changed = true;
    }
    if (oldCloseDoors != closeDoors)
    {
        Serial.println("CloseDoors changed from " + String(oldCloseDoors) + " to " + String(closeDoors));
        changed = true;
    }
    if (oldMaxOpenedDoorB != maxOpenedDoorB)
    {
        Serial.println("MaxOpenedDoorB changed from " + String(oldMaxOpenedDoorB) + " to " + String(maxOpenedDoorB));
        changed = true;
    }
    if (oldMaxClosedDoorB != maxClosedDoorB)
    {
        Serial.println("MaxClosedDoorB changed from " + String(oldMaxClosedDoorB) + " to " + String(maxClosedDoorB));
        changed = true;
    }
    if (oldMaxClosedDoorA != maxClosedDoorA)
    {
        Serial.println("MaxClosedDoorA changed from " + String(oldMaxClosedDoorA) + " to " + String(maxClosedDoorA));
        changed = true;
    }
    if (oldMaxOpenedDoorA != maxOpenedDoorA)
    {
        Serial.println("MaxOpenedDoorA changed from " + String(oldMaxOpenedDoorA) + " to " + String(maxOpenedDoorA));
        changed = true;
    }
    if (changed)
    {
        //char bufStep[6];
        //snprintf(bufStep, sizeof(bufStep), "%5ld", loopStep);

        Serial.println(
            //String(readCounter)
            //+ "/"   + bufStep
            //+ "------"
            + "B: Max Opened: " + String(maxOpenedDoorB)
            + ", Max Closed: "  + String(maxClosedDoorB)
            + ", Motor: "       + String(doStepMotorB)
            + "------"
            + "A Max Closed: "  + String(maxClosedDoorA)
            + ", Max Opened: "  + String(maxOpenedDoorA)
            + ", Motor: "       + String(doStepMotorA)
            // + "------"
            // + ", Open: "   + String(openDoors)
            // + ", Close: "  + String(closeDoors)
            // + "------"
        );
    }
}


void stepperMove()
{
    if (doorsAction == 1)
    {
        digitalWrite(DirA, LOW); // Direction: sens horaire
        digitalWrite(DirB, LOW);
    } else if (doorsAction == 2) {  // close doors
        digitalWrite(DirA, HIGH); // Direction: sens antihoraire
        digitalWrite(DirB, HIGH);
    }

    // Open or close doors is usually done in max 4 seconds,
    // but loop 10000 times with 500 microseconds = 5 seconds
    for (loopStep = 0; loopStep < MaxWaitDoorMoved; loopStep++)
    {
        doStepMotorA=false;
        doStepMotorB=false;

        readSensors();

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

        //Serial.println("Stepping motors. doStepMotorA: " + String(doStepMotorA) + ", doStepMotorB: " + String(doStepMotorB));

        if (doStepMotorA) digitalWrite(StepA, HIGH);
        if (doStepMotorB) digitalWrite(StepB, HIGH);

        if (doStepMotorA || doStepMotorB)
            delayMicroseconds(StepPulse);       // 5 micro seconds

        if (doStepMotorA) digitalWrite(StepA, LOW);
        if (doStepMotorB) digitalWrite(StepB, LOW);

        if ((doStepMotorA || doStepMotorB))
            delayMicroseconds(DelayLow);        // 495 micro seconds

    }
}

void loop()
{
    readSensors();

    doorsAction = 0;
    if (openDoors == HIGH)
        doorsAction = 1;   // Open the doors
    if (closeDoors == HIGH)
        doorsAction = 2;   // Close the doors

    if (doorsAction != 0)
    {
        // don't care about what button was pressed. Force open, wait 10 sec then close

        // Open doors
        //----------------------
        doorsAction = 1;   // Open the doors
        Serial.println("Opening doors");
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_GREEN);
        tft.setCursor(10, 10);
        tft.println(String(loopCounter++));
        tft.setCursor(10, 25);
        tft.println("Opening doors");

        digitalWrite(EnabledA, LOW); // Activation des drivers (LOW = activé)
        digitalWrite(EnabledB, LOW);
        delayMicroseconds(500);        // 500 us
        stepperMove();
        doStepMotorA=false;
        doStepMotorB=false;
        digitalWrite(EnabledA, HIGH); // Desactivation des drivers (HIGH = désactivé)
        digitalWrite(EnabledB, HIGH);

        // wait 10 sec
        //----------------------
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_GREEN);
        tft.setCursor(10, 10);
        tft.println(String(loopCounter++));
        tft.setCursor(10, 25);
        tft.println("Waiting 10 seconds to close doors");
        delay(DelayBeforeClose);       // delay 10 seconds

        // Close doors
        //----------------------
        doorsAction = 2;   // Close the doors
        Serial.println("Closing doors");
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_GREEN);
        tft.setCursor(10, 10);
        tft.println(String(loopCounter++));
        tft.setCursor(10, 25);
        tft.println("Closing doors");

        digitalWrite(EnabledA, LOW); // Activation des drivers (LOW = activé)
        digitalWrite(EnabledB, LOW);
        delayMicroseconds(500);        // 500 us
        stepperMove();
        doStepMotorA=false;
        doStepMotorB=false;
        digitalWrite(EnabledA, HIGH); // Desactivation des drivers (HIGH = désactivé)
        digitalWrite(EnabledB, HIGH);

        Serial.println("Doors closed");
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_GREEN);
        tft.setCursor(10, 10);
        tft.println(String(loopCounter++));
        tft.setCursor(10, 25);
        tft.println("Doors closed");

        loopStep = 0;
    }
    //delay(500);
}
