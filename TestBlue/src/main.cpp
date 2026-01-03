#include <Arduino.h>
#include <NimBLEDevice.h>

NimBLEClient* pClient = nullptr;
bool doConnect = false;
bool connected = false;
NimBLEAddress* pServerAddress = nullptr;

// Standard UUIDs for HID (keyboard/remote)
static BLEUUID serviceUUID("1812"); // Human Interface Device

// Callback for keypress notifications
void notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("🎮 Key detected - Raw data: ");
    for (size_t i = 0; i < length; i++) {
        Serial.printf("0x%02X ", pData[i]);
    }
    Serial.println();

    // Basic interpretation of HID data
    if (length >= 2) {
        uint8_t modifier = pData[0];
        uint8_t keycode = pData[1];

        if (keycode != 0) {
            Serial.printf("   ➜ Keycode: 0x%02X, Modifier: 0x%02X\n", keycode, modifier);
        }
    }
}

// Client event callbacks
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("✓ Connected to remote!");
        connected = true;
    }

    void onDisconnect(NimBLEClient* pClient) {
        Serial.println("✗ Disconnected from remote");
        connected = false;
        // Restart the scan
        Serial.println("Restarting scan...\n");
        delay(1000);
        NimBLEDevice::getScan()->start(0);
    }
};

bool connectToServer() {
    Serial.println("\n=== Attempting connection ===");
    Serial.print("Address: ");
    Serial.println(pServerAddress->toString().c_str());

    if (pClient == nullptr) {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks());
    }

    Serial.println("Connecting...");
    if (!pClient->connect(*pServerAddress)) {
        Serial.println("❌ Connection failed");
        return false;
    }

    Serial.println("✓ Physical connection established");
    delay(1000);

    // List all available services
    Serial.println("\n--- Available services ---");
    auto services = pClient->getServices(true);

    if (services.size() > 0) {
        for (auto service : services) {
            Serial.print("Service UUID: ");
            Serial.println(service->getUUID().toString().c_str());

            // List characteristics of each service
            auto characteristics = service->getCharacteristics(true);
            if (characteristics.size() > 0) {
                for (auto characteristic : characteristics) {
                    Serial.print("  ├─ Characteristic: ");
                    Serial.print(characteristic->getUUID().toString().c_str());
                    Serial.print(" | Properties: ");
                    if (characteristic->canRead()) Serial.print("READ ");
                    if (characteristic->canWrite()) Serial.print("WRITE ");
                    if (characteristic->canNotify()) Serial.print("NOTIFY ");
                    if (characteristic->canIndicate()) Serial.print("INDICATE ");
                    Serial.println();
                }
            }
        }
    } else {
        Serial.println("❌ No services found");
    }
    Serial.println("---------------------------\n");

    // Look for HID service
    NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.println("⚠ HID service (1812) not found");
        Serial.println("The remote may use a different service");
        Serial.println("Check the list above\n");

        // Try subscribing to ALL available notify characteristics
        Serial.println("Attempting to subscribe to all notify characteristics...");
        bool anySubscribed = false;

        if (services.size() > 0) {
            for (auto service : services) {
                auto chars = service->getCharacteristics(true);
                if (chars.size() > 0) {
                    for (auto pChar : chars) {
                        if (pChar->canNotify()) {
                            Serial.print("  ➜ Subscribing to: ");
                            Serial.println(pChar->getUUID().toString().c_str());

                            if (pChar->subscribe(true, notifyCallback)) {
                                Serial.println("    ✓ Subscription succeeded!");
                                anySubscribed = true;
                            } else {
                                Serial.println("    ✗ Failed");
                            }
                        }
                    }
                }
            }
        }

        if (anySubscribed) {
            Serial.println("\n✓ At least one subscription succeeded!");
            return true;
        } else {
            Serial.println("\n❌ No subscriptions succeeded");
            pClient->disconnect();
            return false;
        }
    }

    Serial.println("✓ HID service found!");

    // Subscribe to notify characteristics of the HID service
    auto pCharacteristics = pRemoteService->getCharacteristics(true);

    if (pCharacteristics.size() > 0) {
        Serial.println("Subscribing to HID service notify characteristics...");
        bool subscribed = false;

        for (auto pChar : pCharacteristics) {
            if (pChar->canNotify()) {
                Serial.print("  ➜ Subscribing to: ");
                Serial.println(pChar->getUUID().toString().c_str());

                if (pChar->subscribe(true, notifyCallback)) {
                    Serial.println("    ✓ Subscription succeeded!");
                    subscribed = true;
                } else {
                    Serial.println("    ✗ Failed");
                }
            }
        }

        if (!subscribed) {
            Serial.println("\n❌ No notify subscriptions succeeded");
            pClient->disconnect();
            return false;
        }
    } else {
        Serial.println("❌ No characteristics found");
        pClient->disconnect();
        return false;
    }

    return true;
}

// Callback pour le scan
class ScanCallbacks : public NimBLEScanCallbacks {
    void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.print("📡 Device discovered with Mac address ");
        Serial.print(advertisedDevice->getAddress().toString().c_str());

        if (advertisedDevice->haveName()) {
            Serial.print(" | ");
            Serial.print(advertisedDevice->getName().c_str());
        } else {
            Serial.print(" | (no name)");
        }

        Serial.print(" | RSSI: ");
        Serial.print(advertisedDevice->getRSSI());

        Serial.print(" | Type: ");
        Serial.print(advertisedDevice->getAddressType());

        // Show service UUIDs
        if (advertisedDevice->haveServiceUUID()) {
            Serial.print(" | Services: ");
            for (int i = 0; i < advertisedDevice->getServiceUUIDCount(); i++) {
                Serial.print(advertisedDevice->getServiceUUID(i).toString().c_str());
                Serial.print(" ");
            }
        }

        Serial.println();

        // Detect by known MAC address
        auto addrStr = advertisedDevice->getAddress().toString();
        if (addrStr == "2a:07:98:01:38:9b" || addrStr == "b8:f1:77:e1:7f:96" || addrStr == "99:99:04:04:14:83") {
            Serial.println("\n🎮 >>> REMOTE DETECTED! <<<");
            // Serial.print(">>> MAC Address: ");
            // Serial.println(advertisedDevice->getAddress().toString().c_str());

            NimBLEDevice::getScan()->stop();
            pServerAddress = new NimBLEAddress(advertisedDevice->getAddress());
            doConnect = true;
        }

        // Look for an HID device
        // else if (advertisedDevice->isAdvertisingService(serviceUUID)) {
        //     Serial.println("\n🎮 >>> HID REMOTE DETECTED! <<<");
        //     Serial.print(">>> MAC Address: ");
        //     Serial.println(advertisedDevice->getAddress().toString().c_str());

        //     NimBLEDevice::getScan()->stop();
        //     pServerAddress = new NimBLEAddress(advertisedDevice->getAddress());
        //     doConnect = true;
        // }

        // Detect by name "BLE-M3" or "Yiser-J6" or "Game-pad"
        else if (advertisedDevice->haveName()) {
            String name = String(advertisedDevice->getName().c_str());
            if (name == "BLE-M3" || name == "Yiser-J6" || name == "Game-pad") {
            Serial.println("\n🎮 >>> Remote detected (by name) <<<");
            Serial.print(">>> MAC Address: ");
                Serial.println(advertisedDevice->getAddress().toString().c_str());

                NimBLEDevice::getScan()->stop();
                pServerAddress = new NimBLEAddress(advertisedDevice->getAddress());
                doConnect = true;
            }
        }
    }

    void onScanEnd(NimBLEScanResults results) {
        Serial.println("--- Scan finished ---\n");
    }
};

void setup() {
    Serial.begin(115200);
    delay(5000);

    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  BLE Remote Detection                  ║");
    Serial.println("║  ESP32-C3 + NimBLE 2.3.7               ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("\n");

    // Initialize NimBLE (no security callbacks)
    NimBLEDevice::init("ESP32-C3");

    // Configure the scan
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(new ScanCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->setDuplicateFilter(true); // Avoid repeated logs

    Serial.println("🔍 Starting BLE scan...");
    Serial.println("💡 Put the remote in pairing mode");
    Serial.println("💡 (blinking LED)\n");

    pBLEScan->start(0);
}

void loop() {
    if (doConnect) {
        doConnect = false;
        if (connectToServer()) {
            Serial.println("\n");
            Serial.println("╔════════════════════════════════════════╗");
            Serial.println("║  ✓ CONNECTED AND READY!                ║");
            Serial.println("║  Press buttons...                      ║");
            Serial.println("╚════════════════════════════════════════╝");
            Serial.println("\n");
        } else {
            Serial.println("\n⚠ Failed - restarting scan in 3s...\n");
            delay(3000);
            NimBLEDevice::getScan()->start(0);
        }
    }
    delay(100);
}