#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// 🎯 Adresse MAC de votre télécommande (Game-pad)
static BLEAddress targetDeviceAddress("99:99:04:04:14:83");

static bool doConnect = false;
static bool connected = false;
static BLEClient* pClient = nullptr; // Pointeur pour gérer la connexion client
static BLEScan* pBLEScan = nullptr;

// Classe de rappel pour gérer la connexion
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        Serial.println("Connexion au Game-pad réussie !");
        connected = true;
    }

    void onDisconnect(BLEClient* pclient) {
        Serial.println("Déconnexion du Game-pad.");
        connected = false;
        // On pourrait relancer le scan ici si nécessaire
        doConnect = true; // Tenter de reconnecter
    }
};

// -----------------------------------------------------
// Tente d'établir une connexion BLE avec l'appareil cible.
// -----------------------------------------------------
bool connectToServer() {
    Serial.print("Tentative de connexion à ");
    Serial.println(targetDeviceAddress.toString().c_str());

    if (!pClient->connect(targetDeviceAddress)) {
        Serial.println("Échec de la connexion.");
        return false;
    }

    Serial.println("Connexion établie avec succès.");

    // --- Étape suivante (Non implémentée ici) ---
    // Vous auriez besoin d'obtenir et d'interagir avec les services/caractéristiques
    // du Game-pad, notamment le service HID (Human Interface Device)
    // pour lire les pressions de boutons.
    // Exemple :
    // BLERemoteService* pRemoteService = pClient->getService(BLEUUID("1812")); // Service HID
    // ---------------------------------------------

    return true;
}

// -----------------------------------------------------
// Classe de rappel pour les résultats du scan
// -----------------------------------------------------
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {

        Serial.print("MyAdvertisedDeviceCallbacks, OnResult. RSSI: ");
        Serial.println(advertisedDevice.getRSSI());


        // Filtrer uniquement l'appareil que nous voulons
        if (advertisedDevice.getAddress().equals(targetDeviceAddress))
        {
            Serial.print("Game-pad ciblé trouvé. RSSI: ");
            Serial.println(advertisedDevice.getRSSI());

            // Arrêter le scan et définir le drapeau de connexion
            BLEDevice::getScan()->stop();
            doConnect = true;
        }
    }
};


void setup() {
  Serial.begin(115200);
  delay(5000); // Attendre 5 secondes avant de réessayer
  Serial.println("Démarrage du Client BLE...");

  // 1. Initialiser le périphérique BLE
  BLEDevice::init("");

  // 2. Créer un client BLE et définir les callbacks
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // 3. Obtenir l'objet scanner et définir les callbacks de scan
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Scan actif pour une détection rapide
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  // 4. Si nous avons trouvé l'appareil et ne sommes pas encore connectés, connectons-nous.
  if (doConnect && !connected) {
    if (connectToServer()) {
      doConnect = false; // Connexion réussie, ne pas réessayer immédiatement
    } else {
      Serial.println("Échec de la connexion, tentative dans 5 secondes...");
      delay(5000); // Attendre 5 secondes avant de réessayer
      doConnect = true; // Permettre une nouvelle tentative
    }
  }

  // 5. Si nous ne sommes pas connectés, scanner pour trouver l'appareil cible.
  else if (!connected) {
    Serial.println("Recherche du Game-pad...");
    pBLEScan->start(5); // Scanner pendant 5 secondes
  }

  // 6. Si nous sommes connectés, exécuter la logique principale (lecture des boutons, etc.)
  if (connected) {
    // 💡 Ici, vous auriez la logique pour lire les boutons
    // Pour l'instant, nous attendons juste.
    delay(1000);
  }
}