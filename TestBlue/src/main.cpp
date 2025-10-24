#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>

#define LED_PIN 8  // LED intégrée du SuperMini

// 🎯 Adresse MAC de votre télécommande (Game-pad)
static BLEAddress targetDeviceAddress("99:99:04:04:14:83");

// UUIDs des services et caractéristiques standards HID
static BLEUUID serviceUUID("1812");           // Service HID
static BLEUUID reportCharacteristicUUID("2A4D"); // Caractéristique de Rapport (Report) - Souvent utilisée pour les entrées

static bool doConnect = false;
static bool connected = false;
static BLEClient* pClient = nullptr;
static BLEScan* pBLEScan = nullptr;
static BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;

// -----------------------------------------------------
// 1. Fonction de Callback pour les notifications (Nouvelles données de touche)
// -----------------------------------------------------
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    Serial.println("=============================================");
    Serial.printf("!!! DONNÉES REÇUES de Caractéristique : %s !!!\n", pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print("Nouvelles données de Game-pad (");
    Serial.print(length);
    Serial.print(" octets): ");

    // Affichage des données brutes en hexadécimal
    for (int i = 0; i < length; i++) {
        Serial.printf("%02X ", pData[i]);
    }
    Serial.println();

    // 💡 Décodage : C'est ici que l'on analyse quel octet contient l'état des boutons.
    // L'index exact dépendra du paquet de votre télécommande.
    if (length > 2) {
        uint8_t button_status = pData[2];

        Serial.print("État des boutons (Octet 3): 0x");
        Serial.println(button_status, HEX);

        if (button_status != 0x00) {
            Serial.println("--- Bouton(s) pressé(s) ! ---");
            // Les bits sont souvent utilisés pour les boutons A, B, X, Y
            if (button_status & 0x01) Serial.println("-> Bouton 1/A pressé");
            if (button_status & 0x02) Serial.println("-> Bouton 2/B pressé");
            if (button_status & 0x04) Serial.println("-> Bouton 3/X pressé");
            // ... etc.
        }
    }
    Serial.println("=============================================");

}

// -----------------------------------------------------
// 2. Tente d'établir une connexion BLE avec l'appareil cible et de s'abonner.
// -----------------------------------------------------
bool connectToServer() {
    Serial.print("Tentative de connexion à ");
    Serial.println(targetDeviceAddress.toString().c_str());

    if (!pClient->connect(targetDeviceAddress)) {
        Serial.println("Échec de la connexion.");
        return false;
    }
    Serial.println("Connexion établie avec succès. Découverte de TOUS les services...");

    // Obtenir TOUS les services
    std::map<std::string, BLERemoteService*>* pServices = pClient->getServices();
    if (pServices->empty()) {
        Serial.println("Aucun Service trouvé sur le Game-pad. Déconnexion.");
        pClient->disconnect();
        return false;
    }

    bool subscribed = false;

    // Itérer sur TOUS les services
    for (auto const& pairServices : *pServices) {
        const std::string& uuid = pairServices.first;
        BLERemoteService* pService = pairServices.second;

        Serial.printf("\nSERVICE trouvé: %s\n", pService->toString().c_str());

        // Obtenir TOUTES les caractéristiques de ce service
        std::map<std::string, BLERemoteCharacteristic*>* pCharacteristics = pService->getCharacteristics();

        if (pCharacteristics->empty()) {
            Serial.println("   -> (Aucune Caractéristique)");
            continue;
        }

        // Itérer sur TOUTES les caractéristiques
        //for (auto const& [char_uuid, pChar] : *pCharacteristics) {
        for (const auto& pairCharacteristics : *pCharacteristics)
        {
            const std::string& char_uuid = pairCharacteristics.first;
            BLERemoteCharacteristic* pChar = pairCharacteristics.second;
            if (pChar->canNotify())
            {
                Serial.printf("   -> Caractéristique trouvée :%s\n",pChar->toString().c_str());
                // S'abonner à toute caractéristique capable de notifier
                pChar->registerForNotify(notifyCallback, true);
                subscribed = true;
            }
        }
    }

    if (subscribed) {
        Serial.println("\n*** ABONNEMENT(S) MULTIPLE(S) ACTIF(S) ***");
        Serial.println("Appuyez sur les boutons de la télécommande pour identifier la bonne UUID.");
        return true;
    } else {
        Serial.println("\nAUCUNE Caractéristique supportant les Notifications trouvée. Déconnexion.");
        pClient->disconnect();
        return false;
    }


    /*
    SERVICE trouvé: Service: uuid: 00001812-0000-1000-8000-00805f9b34fb, start_handle: 31 0x001f, end_handle: 74 0x004a
    [ 12184][E][BLERemoteCharacteristic.cpp:289] retrieveDescriptors(): esp_ble_gattc_get_all_descr: Unknown
    [ 12193][E][BLERemoteCharacteristic.cpp:289] retrieveDescriptors(): esp_ble_gattc_get_all_descr: Unknown
    [ 12203][E][BLERemoteCharacteristic.cpp:289] retrieveDescriptors(): esp_ble_gattc_get_all_descr: Unknown
    [ 12212][E][BLERemoteCharacteristic.cpp:289] retrieveDescriptors(): esp_ble_gattc_get_all_descr: Unknown
    [ 12222][E][BLERemoteCharacteristic.cpp:289] retrieveDescriptors(): esp_ble_gattc_get_all_descr: Unknown
    -> Caractéristique trouvée :Characteristic: uuid: 00002a22-0000-1000-8000-00805f9b34fb, handle: 41 0x0029, props: broadcast: 0, read: 1, write_nr: 0, write: 0, notify: 1, indicate: 0, auth: 0
    -> Caractéristique trouvée :Characteristic: uuid: 00002a4d-0000-1000-8000-00805f9b34fb, handle: 46 0x002e, props: broadcast: 0, read: 1, write_nr: 0, write: 0, notify: 1, indicate: 0, auth: 0

    SERVICE trouvé: Service: uuid: f000ffc0-0451-4000-b000-000000000000, start_handle: 75 0x004b, end_handle: 83 0x0053
    -> Caractéristique trouvée :Characteristic: uuid: f000ffc1-0451-4000-b000-000000000000, handle: 77 0x004d, props: broadcast: 0, read: 0, write_nr: 1, write: 1, notify: 1, indicate: 0, auth: 0
    -> Caractéristique trouvée :Characteristic: uuid: f000ffc2-0451-4000-b000-000000000000, handle: 81 0x0051, props: broadcast: 0, read: 0, write_nr: 1, write: 1, notify: 1, indicate: 0, auth: 0


    */


}

// -----------------------------------------------------
// 3. Classes de Callback (Connexion et Scan)
// -----------------------------------------------------

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        // La connexion initiale est gérée dans connectToServer()
    }

    void onDisconnect(BLEClient* pclient) {
        Serial.println("Déconnexion du Game-pad.");
        connected = false;
        doConnect = true; // Tenter de reconnecter
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // Filtrer uniquement l'appareil cible
        if (advertisedDevice.getAddress().equals(targetDeviceAddress)) {
            Serial.print("Game-pad ciblé trouvé. RSSI: ");
            Serial.println(advertisedDevice.getRSSI());

            BLEDevice::getScan()->stop();
            doConnect = true;
        }
    }
};


void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    delay(5000);
    Serial.println("Démarrage du Client BLE...");

    BLEDevice::init("");

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(300);

    digitalWrite(LED_PIN, LOW);
    delay(300);

    // 4. Logique de Connexion/Reconnexion
    if (doConnect && !connected) {
        if (connectToServer()) {
            doConnect = false;
            connected = true; // Marquer la connexion comme établie APRÈS l'abonnement
        } else {
            Serial.println("Échec de la connexion/abonnement, nouvelle tentative dans 5 secondes...");
            delay(5000);
            // doConnect reste à true pour relancer une tentative
        }
    }

    // 5. Logique de Scan (uniquement si non connecté)
    else if (!connected) {
        Serial.println("Recherche du Game-pad...");
        pBLEScan->start(5, false); // Scanner pendant 5 secondes
    }

    // 6. Logique Principale (si connecté)
    if (connected) {
        // Toutes les interactions (pressions de boutons) se font via le notifyCallback
        // La boucle principale n'a plus qu'à maintenir l'exécution
        delay(100);
    }
}
