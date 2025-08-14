#include <Arduino.h>

// Bike Unit ESP32 BLE Client
#include <BLEDevice.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_TX        "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_UUID_RX        "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID txUUID(CHAR_UUID_TX);
static BLEUUID rxUUID(CHAR_UUID_RX);

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* txCharacteristic;
static BLERemoteCharacteristic* rxCharacteristic;
static BLEAdvertisedDevice* myDevice;

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData, size_t length, bool isNotify) {
    String data = String((char*)pData);
    Serial.println("Received from Helmet: " + data);

    if (data == "true") {
        Serial.println("Sending ACK to Helmet");
        rxCharacteristic->writeValue("ack");
    }
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
        }
    }
};

bool connectToServer() {
    BLEClient* pClient = BLEDevice::createClient();
    if (!pClient->connect(myDevice)) return false;

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) return false;

    txCharacteristic = pRemoteService->getCharacteristic(txUUID);
    rxCharacteristic = pRemoteService->getCharacteristic(rxUUID);

    if (txCharacteristic) txCharacteristic->registerForNotify(notifyCallback);
    connected = true;
    return true;
}

void setup() {
    Serial.begin(115200);
    BLEDevice::init("BikeUnit");

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void loop() {
    if (doConnect) {
        if (connectToServer()) {
            Serial.println("Connected to Helmet Unit.");
        }
        doConnect = false;
    }
}
