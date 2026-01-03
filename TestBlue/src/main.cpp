#include <Arduino.h>
#include <NimBLEDevice.h>

NimBLEClient* pClient = nullptr;
bool doConnect = false;
bool connected = false;
NimBLEAddress* pServerAddress = nullptr;

// UUIDs standards pour HID (clavier/télécommande)
static BLEUUID serviceUUID("1812"); // Human Interface Device

// Callback pour les notifications des touches pressées
void notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("🎮 Touche détectée - Données brutes: ");
    for (size_t i = 0; i < length; i++) {
        Serial.printf("0x%02X ", pData[i]);
    }
    Serial.println();

    // Interprétation basique des données HID
    if (length >= 2) {
        uint8_t modifier = pData[0];
        uint8_t keycode = pData[1];

        if (keycode != 0) {
            Serial.printf("   ➜ Code touche: 0x%02X, Modificateur: 0x%02X\n", keycode, modifier);
        }
    }
}

// Callback pour les événements du client
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("✓ Connecté à la télécommande!");
        connected = true;
    }

    void onDisconnect(NimBLEClient* pClient) {
        Serial.println("✗ Déconnecté de la télécommande");
        connected = false;
        // Redémarrer le scan
        Serial.println("Redémarrage du scan...\n");
        delay(1000);
        NimBLEDevice::getScan()->start(0);
    }
};

bool connectToServer() {
    Serial.println("\n=== Tentative de connexion ===");
    Serial.print("Adresse: ");
    Serial.println(pServerAddress->toString().c_str());

    if (pClient == nullptr) {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks());
    }

    Serial.println("Connexion en cours...");
    if (!pClient->connect(*pServerAddress)) {
        Serial.println("❌ Échec de connexion");
        return false;
    }

    Serial.println("✓ Connexion physique établie");
    delay(1000);

    // Lister tous les services disponibles
    Serial.println("\n--- Services disponibles ---");
    auto services = pClient->getServices(true);

    if (services.size() > 0) {
        for (auto service : services) {
            Serial.print("Service UUID: ");
            Serial.println(service->getUUID().toString().c_str());

            // Lister les caractéristiques de chaque service
            auto characteristics = service->getCharacteristics(true);
            if (characteristics.size() > 0) {
                for (auto characteristic : characteristics) {
                    Serial.print("  ├─ Caractéristique: ");
                    Serial.print(characteristic->getUUID().toString().c_str());
                    Serial.print(" | Propriétés: ");
                    if (characteristic->canRead()) Serial.print("READ ");
                    if (characteristic->canWrite()) Serial.print("WRITE ");
                    if (characteristic->canNotify()) Serial.print("NOTIFY ");
                    if (characteristic->canIndicate()) Serial.print("INDICATE ");
                    Serial.println();
                }
            }
        }
    } else {
        Serial.println("❌ Aucun service trouvé");
    }
    Serial.println("---------------------------\n");

    // Chercher le service HID
    NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.println("⚠ Service HID (1812) non trouvé");
        Serial.println("La télécommande utilise peut-être un autre service");
        Serial.println("Vérifiez la liste ci-dessus\n");

        // Essayer de s'abonner à TOUTES les caractéristiques notify disponibles
        Serial.println("Tentative d'abonnement à toutes les caractéristiques notify...");
        bool anySubscribed = false;

        if (services.size() > 0) {
            for (auto service : services) {
                auto chars = service->getCharacteristics(true);
                if (chars.size() > 0) {
                    for (auto pChar : chars) {
                        if (pChar->canNotify()) {
                            Serial.print("  ➜ Abonnement à: ");
                            Serial.println(pChar->getUUID().toString().c_str());

                            if (pChar->subscribe(true, notifyCallback)) {
                                Serial.println("    ✓ Abonnement réussi!");
                                anySubscribed = true;
                            } else {
                                Serial.println("    ✗ Échec");
                            }
                        }
                    }
                }
            }
        }

        if (anySubscribed) {
            Serial.println("\n✓ Au moins un abonnement réussi!");
            return true;
        } else {
            Serial.println("\n❌ Aucun abonnement réussi");
            pClient->disconnect();
            return false;
        }
    }

    Serial.println("✓ Service HID trouvé!");

    // S'abonner aux caractéristiques notify du service HID
    auto pCharacteristics = pRemoteService->getCharacteristics(true);

    if (pCharacteristics.size() > 0) {
        Serial.println("Abonnement aux caractéristiques notify du service HID...");
        bool subscribed = false;

        for (auto pChar : pCharacteristics) {
            if (pChar->canNotify()) {
                Serial.print("  ➜ Abonnement à: ");
                Serial.println(pChar->getUUID().toString().c_str());

                if (pChar->subscribe(true, notifyCallback)) {
                    Serial.println("    ✓ Abonnement réussi!");
                    subscribed = true;
                } else {
                    Serial.println("    ✗ Échec");
                }
            }
        }

        if (!subscribed) {
            Serial.println("\n❌ Aucun abonnement notify réussi");
            pClient->disconnect();
            return false;
        }
    } else {
        Serial.println("❌ Aucune caractéristique trouvée");
        pClient->disconnect();
        return false;
    }

    return true;
}

// Callback pour le scan
class ScanCallbacks : public NimBLEScanCallbacks {
    void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.print("📡 ");
        Serial.print("==== OnDiscovered ==== \n");
        Serial.print(advertisedDevice->getAddress().toString().c_str());

        if (advertisedDevice->haveName()) {
            Serial.print(" | ");
            Serial.print(advertisedDevice->getName().c_str());
        } else {
            Serial.print(" | (pas de nom)");
        }

        Serial.print(" | RSSI: ");
        Serial.print(advertisedDevice->getRSSI());

        // Afficher le type d'adresse
        Serial.print(" | Type: ");
        Serial.print(advertisedDevice->getAddressType());

        // Afficher les UUIDs de service
        if (advertisedDevice->haveServiceUUID()) {
            Serial.print(" | Services: ");
            for (int i = 0; i < advertisedDevice->getServiceUUIDCount(); i++) {
                Serial.print(advertisedDevice->getServiceUUID(i).toString().c_str());
                Serial.print(" ");
            }
        }

        Serial.println();

        // Détecter par adresse MAC connue (votre télécommande BLE-M3)
        if (advertisedDevice->getAddress().toString() == "2a:07:98:01:38:9b") {
            Serial.println("\n🎮 >>> TÉLÉCOMMANDE BLE-M3 DÉTECTÉE! <<<");
            Serial.print(">>> Adresse MAC: ");
            Serial.println(advertisedDevice->getAddress().toString().c_str());

            NimBLEDevice::getScan()->stop();
            pServerAddress = new NimBLEAddress(advertisedDevice->getAddress());
            doConnect = true;
        }
        // Chercher un appareil HID
        else if (advertisedDevice->isAdvertisingService(serviceUUID)) {
            Serial.println("\n🎮 >>> TÉLÉCOMMANDE HID DÉTECTÉE! <<<");
            Serial.print(">>> Adresse MAC: ");
            Serial.println(advertisedDevice->getAddress().toString().c_str());

            NimBLEDevice::getScan()->stop();
            pServerAddress = new NimBLEAddress(advertisedDevice->getAddress());
            doConnect = true;
        }
        // Détecter par nom "BLE-M3"
        else if (advertisedDevice->haveName()) {
            String name = String(advertisedDevice->getName().c_str());
            if (name == "BLE-M3" || name.indexOf("remote") >= 0 ||
                name.indexOf("keyboard") >= 0 || name.indexOf("kb") >= 0) {
                Serial.println("\n🎮 >>> Télécommande détectée (par nom) <<<");
                Serial.print(">>> Adresse MAC: ");
                Serial.println(advertisedDevice->getAddress().toString().c_str());

                NimBLEDevice::getScan()->stop();
                pServerAddress = new NimBLEAddress(advertisedDevice->getAddress());
                doConnect = true;
            }
        }
    }

    void onScanEnd(NimBLEScanResults results) {
        Serial.println("--- Scan terminé ---\n");
    }
};

void setup() {
    Serial.begin(115200);
    delay(5000);

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  Détection Télécommande BLE           ║");
    Serial.println("║  ESP32-C3 + NimBLE 2.3.7              ║");
    Serial.println("╚════════════════════════════════════════╝\n");

    // Initialiser NimBLE (sans callbacks de sécurité)
    NimBLEDevice::init("ESP32-C3");

    // Configurer le scan
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(new ScanCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->setDuplicateFilter(true); // Éviter les logs répétés

    Serial.println("🔍 Démarrage du scan BLE...");
    Serial.println("💡 Mettez la télécommande en mode appairage");
    Serial.println("💡 (LED clignotante)\n");

    pBLEScan->start(0);
}

void loop() {
    if (doConnect) {
        doConnect = false;
        if (connectToServer()) {
            Serial.println("\n╔════════════════════════════════════════╗");
            Serial.println("║  ✓ CONNECTÉ ET PRÊT!                  ║");
            Serial.println("║  Appuyez sur les touches...           ║");
            Serial.println("╚════════════════════════════════════════╝\n");
        } else {
            Serial.println("\n⚠ Échec - Reprise du scan dans 3s...\n");
            delay(3000);
            NimBLEDevice::getScan()->start(0);
        }
    }

    delay(100);
}