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
bool doScan = false;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    Serial.println("Connected to Helmet.");
  }
  void onDisconnect(BLEClient* pClient) {
    connected = false;
    Serial.println("⚠️ Disconnected from Helmet. Waiting for reconnection...");
    doScan = true;  // trigger scan again
  }
};
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.println("✅Helmet found!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                           uint8_t* pData, size_t length, bool isNotify) {
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)pData[i];

  if (msg == "true") {
    Serial.println("✅ Helmet secure: Worn & Buckled");
  } else if (msg == "warn") {
    Serial.println("⚠️ Warning: Helmet not worn or buckle open!");
  }
}
bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  if (!pClient->connect(myDevice)) return false;

  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) return false;

  txCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHAR_UUID_TX));
  rxCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHAR_UUID_RX));

  if (txCharacteristic->canNotify()) {
    txCharacteristic->registerForNotify(notifyCallback);
  }

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
  pScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("✅ Successfully connected to Helmet.");
    } else {
      Serial.println("❌ Connection failed. Will rescan...");
    }
    doConnect = false;
  }

  if (doScan && !connected) {
    Serial.println("🔍 Scanning for Helmet...");
    BLEDevice::getScan()->start(5, false);  // scan for 5 seconds
    doScan = false;
  }

  if (connected) {
    if (digitalRead(STAND_PIN) == HIGH) {
      rxCharacteristic->writeValue("true");
      Serial.println("📤Stand sensor TRUE sent to Helmet");
    }
    delay(500);
    } else {
    delay(1000); // reduce CPU use while waiting
  }
}
