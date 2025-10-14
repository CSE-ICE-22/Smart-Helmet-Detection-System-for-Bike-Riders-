#include <Arduino.h>
#include <BLEDevice.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);  

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
    doScan = true;
  }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
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
  unsigned long receiptTime = millis();  // Time notification received

  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)pData[i];

  if (msg == "true") {
    Serial.println("✅ Helmet secure: Worn & Buckled");
    lcd.setCursor(0, 1);
    lcd.print("Helmet Secure   ");
  } else if (msg == "warn") {
    Serial.println("⚠️ Warning: Helmet not worn or buckle open!");
    lcd.setCursor(0, 1);
    lcd.print("Helmet Warning! ");
  }

  // Assume helmet logs send time; compute delta manually or sync clocks.
  // For simplicity, log receipt time; calculate end-to-end offline.
  Serial.printf("Notification received: %s, Receipt Time: %lu ms\n", msg.c_str(), receiptTime);
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
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
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
      lcd.setCursor(0, 0);
      lcd.print("Helmet connected.");
      delay(1000);
    } else {
      Serial.println("❌ Connection failed. Will rescan...");
      lcd.setCursor(0, 0);
      lcd.print("Connection faild.");
    }
    doConnect = false;
  }

  if (doScan && !connected) {
    Serial.println("🔍 Scanning for Helmet...");
    BLEDevice::getScan()->start(5, false);
    doScan = false;
  }

  if (connected) {
    if (digitalRead(STAND_PIN) == HIGH) {
      rxCharacteristic->writeValue("true");
      Serial.println("📤Stand sensor TRUE sent to Helmet");
    }
    delay(500);
  } else {
    delay(1000);
  }
}