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
#define OpenDoors_Pin        5
#define CloseDoors_Pin       6

// door 1 detection
#define MaxOpenedDoorA_Pin   9
#define MaxClosedDoorA_Pin   10

// Motor A
#define StepA_Pin            11
#define DirA_Pin             12
#define EnabledA_Pin         13

// door 2 detection
#define MaxOpenedDoorB_Pin   14
#define MaxClosedDoorB_Pin   15

// Motor B
#define StepB_Pin            16
#define DirB_Pin             17
#define EnabledB_Pin         18

// Delays
#define MaxWaitDoorMoved 10000   // 10000 * 500 micro seconds = 5 seconds open or close
#define StepPulseHigh        5   // micro seconds
#define StepPulseLow       700   // micro seconds
#define DelayBeforeClose    10   // seconds


//int readCounter    = 0;
//int loopCounter    = 0;
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
    tft.setTextSize(2); // 1 Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc

    tft.setCursor(10, 10);
    tft.println("ESP32 Doors");

    tft.setCursor(10, 25);
    tft.println("Waiting command");

    // tft.setTextColor(ST77XX_WHITE);
    // for (int i = 0; i < 11; i++)
    // {
    //     tft.setCursor(10, 10 + i * 10);
    //     tft.println("Line " + String(i + 2));
    // }
    // tft.setTextSize(2); // 1 Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc

    delay(100);

    // Serial.println("Ecran prêt !");
    Serial.println("init Gpios");

    // Initialisation Gpio
    pinMode(StepA_Pin, OUTPUT);     // 11
    pinMode(DirA_Pin, OUTPUT);      // 12
    pinMode(EnabledA_Pin, OUTPUT);  // 13

    pinMode(StepB_Pin, OUTPUT);     // 16
    pinMode(DirB_Pin, OUTPUT);      // 17
    pinMode(EnabledB_Pin, OUTPUT);  // 18

    pinMode(OpenDoors_Pin, INPUT_PULLDOWN);   //5
    pinMode(CloseDoors_Pin, INPUT_PULLDOWN);   //6
    pinMode(MaxOpenedDoorA_Pin, INPUT_PULLDOWN);   //9
    pinMode(MaxClosedDoorA_Pin, INPUT_PULLDOWN);   //10
    pinMode(MaxOpenedDoorB_Pin, INPUT_PULLDOWN);   //14
    pinMode(MaxClosedDoorB_Pin, INPUT_PULLDOWN);   //15
    delay(100);

    // digitalWrite(EnabledA_Pin, HIGH); // Desactivation des drivers (HIGH = désactivé)
    // digitalWrite(EnabledB_Pin, HIGH);

    digitalWrite(EnabledA_Pin, LOW); // Activation des drivers (LOW = activé)
    digitalWrite(EnabledB_Pin, LOW);
    delay(100);

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

    openDoors      = digitalRead(OpenDoors_Pin);       // 5
    closeDoors     = digitalRead(CloseDoors_Pin);      // 6
    maxOpenedDoorB = digitalRead(MaxOpenedDoorB_Pin);  // 14
    maxClosedDoorB = digitalRead(MaxClosedDoorB_Pin);  // 15
    maxClosedDoorA = digitalRead(MaxClosedDoorA_Pin);  // 10
    maxOpenedDoorA = digitalRead(MaxOpenedDoorA_Pin);  // 9

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
        digitalWrite(EnabledA_Pin, LOW); // Activation des drivers (LOW = activé)
        digitalWrite(EnabledB_Pin, LOW);

        if (doStepMotorA) digitalWrite(StepA_Pin, HIGH);
        if (doStepMotorB) digitalWrite(StepB_Pin, HIGH);

        if (doStepMotorA || doStepMotorB)
            delayMicroseconds(StepPulseHigh);       // 5 micro seconds

        if (doStepMotorA) digitalWrite(StepA_Pin, LOW);
        if (doStepMotorB) digitalWrite(StepB_Pin, LOW);

        if ((doStepMotorA || doStepMotorB))
            delayMicroseconds(StepPulseLow);        // 495 micro seconds

    }
}

void OpenDoors()
{
    // Open doors
    //----------------------
    doorsAction = 1;   // Open the doors
    Serial.println("Opening doors");
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);
    //tft.setCursor(10, 10);
    //tft.println(String(loopCounter++));
    tft.setCursor(10, 25);
    tft.println("Opening doors");

    digitalWrite(EnabledA_Pin, LOW); // Activation des drivers (LOW = activé)
    digitalWrite(EnabledB_Pin, LOW);

    digitalWrite(DirA_Pin, LOW); // Direction: sens horaire
    digitalWrite(DirB_Pin, LOW);

    delay(100);

    stepperMove();
    doStepMotorA=false;
    doStepMotorB=false;
    //digitalWrite(EnabledA_Pin, HIGH); // Desactivation des drivers (HIGH = désactivé)
    //digitalWrite(EnabledB_Pin, HIGH);
    doorsAction = 0;
}

void CloseDoors()
{
    // Close doors
    //----------------------
    doorsAction = 2;   // Close the doors
    Serial.println("Closing doors");
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);
    //tft.setCursor(10, 10);
    //tft.println(String(loopCounter++));
    tft.setCursor(10, 25);
    tft.println("Closing doors");

    // Activation des drivers (LOW = activé)
    digitalWrite(EnabledA_Pin, LOW);
    digitalWrite(EnabledB_Pin, LOW);

    digitalWrite(DirA_Pin, HIGH); // Direction: sens antihoraire
    digitalWrite(DirB_Pin, HIGH);

    delay(100);

    stepperMove();
    doStepMotorA=false;
    doStepMotorB=false;

    // Desactivation des drivers (HIGH = désactivé)
    digitalWrite(EnabledA_Pin, HIGH);
    digitalWrite(EnabledB_Pin, HIGH);
    doorsAction = 0;
}

void Wait10Seconds()
{
    // wait 10 sec
    //----------------------
    for (int sec = DelayBeforeClose; sec > 0; sec--)
    {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_GREEN);
        //tft.setCursor(10, 10);
        //tft.println(String(loopCounter++));
        tft.setCursor(10, 25);
        tft.println("Wait " + String(sec) + " seconds");
        delay(1000);       // delay 1 second
    }
}

void ShowDoorClosed()
{
    Serial.println("Doors closed");
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);
    //tft.setCursor(10, 10);
    //tft.println(String(loopCounter++));
    tft.setCursor(10, 25);
    tft.println("Doors closed");
}

void loop()
{
    readSensors();

    if (openDoors == HIGH)
    {
        OpenDoors();
        Wait10Seconds();
        CloseDoors();
        ShowDoorClosed();
    }

    if (closeDoors == HIGH)
    {
        CloseDoors();
        ShowDoorClosed();
    }
}
