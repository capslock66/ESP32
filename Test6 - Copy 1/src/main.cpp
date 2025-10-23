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

// Moteur
#define STEP 11
#define DIR 12
#define EN 13
#define STEP_PULSE_US 5

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

    // Initialisation des broches
    pinMode(STEP, OUTPUT);
    pinMode(DIR, OUTPUT);
    pinMode(EN, OUTPUT);

    // Activation des drivers (LOW = activé)
    digitalWrite(EN, LOW);

    // Direction initiale
    digitalWrite(DIR, HIGH); // sens horaire

    delay(10);
}

void stepperMoveRate(float revs_per_sec, unsigned int microstepDen, long stepsToDo )
{
    const unsigned int fullStepsPerRev = 200;
    float steps_per_sec = revs_per_sec * fullStepsPerRev * microstepDen;
    if (steps_per_sec <= 0.0)
        return;

    // période totale en µs par pas
    float period_us = 1e6 / steps_per_sec;

    unsigned long rest_us = (unsigned long)round(period_us) - STEP_PULSE_US;
    if (rest_us < 0)
        rest_us = 0;

    long steps = stepsToDo;
    while (steps != 0)
    {
        digitalWrite(STEP, HIGH);
        delayMicroseconds(STEP_PULSE_US);
        digitalWrite(STEP, LOW);
        delayMicroseconds(rest_us);
        if (steps > 0)
            steps--;
    }
}

int loopCounter = 0;
void loop()
{
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);

    tft.setCursor(10, 10);
    tft.println(String(loopCounter++) + " " + String(millis()) + " ms"); // Shows elapsed ms instead of compile time

    // digitalWrite(DIR_A, HIGH); // sens horaire
    // for (int i = 0; i < 200; i++) { // 200 pas = 1 tour si 1.8°/pas
    //     digitalWrite(STEP_A, HIGH);
    //     //digitalWrite(STEP_B, HIGH);
    //     delayMicroseconds(500); // ajuster pour la vitesse
    //     digitalWrite(STEP_A, LOW);
    //     //digitalWrite(STEP_B, LOW);
    //     delayMicroseconds(500);
    // }

    // digitalWrite(DIR, LOW); // sens antihoraire
    // for (int i = 0; i < 1600; i++) { // 200 pas = 1 tour si 1.8°/pas
    //     digitalWrite(STEP, HIGH);
    //     delayMicroseconds(5); // ajuster pour la vitesse
    //     digitalWrite(STEP, LOW);
    //     delayMicroseconds(625);
    // }

    stepperMoveRate(
        1.0,         // revs_per_sec
        2,           // microstepDen (1/2 microstep)
        400);        // stepsToDo (200*2 = 400 pour 1 tour)

    delay(1000);
}
