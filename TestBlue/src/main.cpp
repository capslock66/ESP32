#include <Arduino.h>
#include <NimBLEDevice.h>

NimBLEClient* pClient = nullptr;
bool doConnect = false;
bool connected = false;
NimBLEAddress* pDeviceAddress = nullptr;

// Standard UUIDs for HID (keyboard/remote)
static BLEUUID serviceUUID("1812"); // Human Interface Device

// Callback for keypress notifications
void characteristicNotifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
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


// Scan device Callback
class ScanDeviceCallbacks : public NimBLEScanCallbacks
{
    // virtual void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice);
    // virtual void onResult(const NimBLEAdvertisedDevice* advertisedDevice);
    // virtual void onScanEnd(const NimBLEScanResults& scanResults, int reason);


    void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice)
    {
        Serial.print("📡 Device discovered with Mac address ");
        Serial.print(advertisedDevice->getAddress().toString().c_str());

        if (advertisedDevice->haveName())
            Serial.printf(" | %15s", advertisedDevice->getName().c_str());
        else
            Serial.printf(" | %15s", "(no name)");

        Serial.printf(" | RSSI: %5d", advertisedDevice->getRSSI());
        Serial.printf(" | Type: %2d", advertisedDevice->getAddressType());

        // Show service UUIDs
        if (advertisedDevice->haveServiceUUID())
        {
            Serial.print(" | Services: ");
            for (int i = 0; i < advertisedDevice->getServiceUUIDCount(); i++)
            {
                Serial.print(advertisedDevice->getServiceUUID(i).toString().c_str());
                Serial.print(" ");
            }
        }

        Serial.println();

        // Detect by known MAC address
        auto addrStr = advertisedDevice->getAddress().toString();
        if (addrStr == "2a:07:98:01:38:9b" ||   // white remote control
            addrStr == "b8:f1:77:e1:7f:96" ||   // ring
            addrStr == "99:99:04:04:14:83") {   // black remote control
            Serial.println("\n🎮 >>> REMOTE DETECTED! <<<");
            // Serial.print(">>> MAC Address: ");
            // Serial.println(advertisedDevice->getAddress().toString().c_str());

            NimBLEDevice::getScan()->stop();
            pDeviceAddress = new NimBLEAddress(advertisedDevice->getAddress());
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
            if (name == "BLE-M3" ||     // white remote control
                name == "Yiser-J6" ||   // ring
                name == "Game-pad") {   // black remote control
                Serial.println("\n🎮 >>> Remote detected (by name) <<<");
                Serial.print(">>> MAC Address: ");
                Serial.println(advertisedDevice->getAddress().toString().c_str());

                NimBLEDevice::getScan()->stop();
                pDeviceAddress = new NimBLEAddress(advertisedDevice->getAddress());
                doConnect = true;
            }
        }
    } // onDiscovered

    void onScanEnd(NimBLEScanResults results) {
        Serial.println("--- Scan finished ---\n");
    }

}; // ScanCallbacks

// Client (remote control) event callbacks
class ClientCallbacks : public NimBLEClientCallbacks
{
    // virtual void onConnect(NimBLEClient* pClient);
    // virtual void onConnectFail(NimBLEClient* pClient, int reason);
    // virtual void onDisconnect(NimBLEClient* pClient, int reason);
    // virtual bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params);
    // virtual void onPassKeyEntry(NimBLEConnInfo& connInfo);
    // virtual void onAuthenticationComplete(NimBLEConnInfo& connInfo);
    // virtual void onConfirmPasskey(NimBLEConnInfo& connInfo, uint32_t pin);
    // virtual void onIdentity(NimBLEConnInfo& connInfo);
    // virtual void onMTUChange(NimBLEClient* pClient, uint16_t MTU);
    // virtual void onPhyUpdate(NimBLEClient* pClient, uint8_t txPhy, uint8_t rxPhy);

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

void PrintAllServicesAndCharacteristics(NimBLEClient* pClient)
{
    Serial.println("\n--- Services and Characteristics ---");

    auto services = pClient->getServices(true);
    for (auto service : services)
    {
        Serial.print("Service: ");
        Serial.println(service->getUUID().toString().c_str());

        auto characteristics = service->getCharacteristics(true);
        for (auto characteristic : characteristics)
        {
            Serial.print("  ➜ Characteristic: ");
            Serial.print(characteristic->getUUID().toString().c_str());
            Serial.print(" | Properties: ");

            if (characteristic->canRead()) Serial.print("Read ");
            if (characteristic->canWrite()) Serial.print("Write ");
            if (characteristic->canWriteNoResponse()) Serial.print("WriteNoResp ");
            if (characteristic->canNotify()) Serial.print("Notify ");
            if (characteristic->canIndicate()) Serial.print("Indicate ");

            Serial.println();
            //Serial.println(characteristic->toString().c_str());
        }
    }
    Serial.println("--- End of Services ---\n");
}

bool RegisterToAllCharacteristics(NimBLERemoteService* pService)
{
    auto pCharacteristics = pService->getCharacteristics(true);

    if (pCharacteristics.size() > 0)
    {
        Serial.println("Subscribing to HID service notify characteristics...");
        bool subscribed = false;

        for (auto pChar : pCharacteristics)
        {
            if (pChar->canNotify())
            {
                Serial.print("  ➜ Subscribing to: ");
                Serial.println(pChar->getUUID().toString().c_str());

                if (pChar->subscribe(true, characteristicNotifyCallback))
                {
                    Serial.println("    ✓ Subscription succeeded!");
                    subscribed = true;
                }
                else
                {
                    Serial.println("    ✗ Failed");
                }
            }
        }
        if (subscribed) {
            Serial.println("\n✓ At least one subscription succeeded!");
            return true;
        }
        Serial.println("\n❌ No subscriptions succeeded");
        pClient->disconnect();
        return false;
    }
}

// create pClient
// create ClientCallbacks => set connected to true when connected, set to false when disconnected
// List available services and characteristics
// Subscribe to all notify characteristics for HID service 1812 (Human Interface Device).
// If not found Subscribe to all notify characteristics of all services
// characteristicNotifyCallback() is called on service Characteristic notifications

bool connectToRemoteControl()
{
    Serial.println("\n=== Attempting connection ===");
    Serial.print("Address: ");
    Serial.println(pDeviceAddress->toString().c_str());

    pClient = NimBLEDevice::createClient();
    // set connected to true when connected, set to false when disconnected
    pClient->setClientCallbacks(new ClientCallbacks());

    Serial.println("Connecting...");
    if (!pClient->connect(*pDeviceAddress))
    {
        Serial.println("❌ Connection failed");
        return false;
    }

    Serial.println("✓ Physical connection established");
    delay(1000);

    PrintAllServicesAndCharacteristics(pClient);

    // Look for HID service 1812 : Human Interface Device
    NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService != nullptr)
    {
        Serial.println("✓ HID service found!");
        return RegisterToAllCharacteristics(pRemoteService); // Subscribe to notify characteristics of the HID service
    }

    Serial.println("⚠ HID service (1812) not found");
    Serial.println("The remote may use a different service");
    Serial.println("Check the list above\n");

    Serial.println("Attempting to subscribe to all notify characteristics for all services...");
    bool anySubscribed = false;
    auto services = pClient->getServices(true);
    if (services.size() > 0)
        for (auto service : services)
            anySubscribed |= RegisterToAllCharacteristics(service); // Subscribe to all notify characteristics of all services

    if (anySubscribed)
    {
        Serial.println("\n✓ At least one subscription succeeded!");
        return true;
    }
    Serial.println("\n❌ No subscriptions succeeded");
    pClient->disconnect();
    return false;
 }



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
    pBLEScan->setScanCallbacks(new ScanDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->setDuplicateFilter(true); // Avoid repeated logs

    Serial.println("🔍 Starting BLE scan...");
    Serial.println("💡 Put the remote in pairing mode");
    Serial.println("💡 (blinking LED)\n");

    pBLEScan->start(0); // wait until device is connected
}

void loop() {
    if (doConnect)
    {
        doConnect = false;
        if (!connectToRemoteControl())  // subscribe to remote control service characteristic notifications
        {
            Serial.println("\n⚠ Failed - restarting scan in 3s...\n");
            delay(3000);
            NimBLEDevice::getScan()->start(0);
            return;
        }
        Serial.println("\n");
        Serial.println("╔════════════════════════════════════════╗");
        Serial.println("║  ✓ CONNECTED AND READY!                ║");
        Serial.println("║  Press buttons...                      ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.println("\n");
    }

    // already connected.
    // do some stuff here

    delay(100);
}