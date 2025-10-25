#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Durée du scan en secondes
#define SCAN_DURATION 5
#define LED_PIN 8  // LED intégrée du SuperMini

BLEScan* pBLEScan;

// Classe pour gérer les résultats du scan
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      // Afficher l'adresse (MAC) et le nom de l'appareil
      Serial.printf("Appareil BLE détecté: %s (MAC: %s, RSSI: %d)\n",
                     advertisedDevice.getName().c_str(),
                     advertisedDevice.getAddress().toString().c_str(),
                     advertisedDevice.getRSSI());

      // Vous pouvez ajouter une logique ici pour vérifier si l'appareil est votre télécommande
      // Exemple : if (advertisedDevice.getName() == "VR_Remote") { ... }
    }
};

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(5000);
  Serial.println("Démarrage du scanner BLE...");

  // Initialiser le périphérique BLE
  BLEDevice::init(""); // Le nom n'est pas nécessaire pour un scan client

  // Obtenir l'objet scanner
  pBLEScan = BLEDevice::getScan();

  // Définir les callbacks pour les résultats du scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  // Définir les paramètres du scan: balayage passif pour la consommation d'énergie
  pBLEScan->setActiveScan(false); // Balayage passif (moins d'énergie)
  pBLEScan->setInterval(100);    // Intervalle entre les scans (ms)
  pBLEScan->setWindow(99);       // Fenêtre de scan (ms)
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(300);

    digitalWrite(LED_PIN, LOW);
    delay(300);	
    Serial.println("Scan démarré...");

  // Lancer le scan pour la durée définie
  BLEScanResults foundDevices = pBLEScan->start(SCAN_DURATION, false);

  Serial.printf("Scan terminé. Appareils trouvés: %d\n", foundDevices.getCount());
  pBLEScan->clearResults();   // Vider les résultats du scan précédents

  // Attendre avant de scanner à nouveau 
  delay(5000);
}