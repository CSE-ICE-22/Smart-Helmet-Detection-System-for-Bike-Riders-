#include <Arduino.h>
#include <BLEDevice.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define STAND_PIN 26

static BLERemoteCharacteristic* txCharacteristic;
static BLERemoteCharacteristic* rxCharacteristic;
static BLEAdvertisedDevice* myDevice;

bool connected = false;
bool doConnect = false;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.println("Helmet found!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  if (!pClient->connect(myDevice)) return false;

  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) return false;

  txCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHAR_UUID_TX));
  rxCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHAR_UUID_RX));
  connected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(STAND_PIN, INPUT);

  BLEDevice::init("BikeUnit");

  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
 
  pScan->setActiveScan(true);
  pScan->start(20, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to Helmet.");
    } else {
      Serial.println("Connection failed.");
    }
    doConnect = false;
  }

  if (connected) {
    if (digitalRead(STAND_PIN) == HIGH) {
      rxCharacteristic->writeValue("true");
      Serial.println("Stand sensor TRUE sent to Helmet");
    }
    delay(500);
  }
}
