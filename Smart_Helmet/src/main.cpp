#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Callback for server connection events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
    }
};

// Callback for characteristic write events
// Callback for receiving data
class MyCallbacks : public BLECharacteristicCallbacks {
  String receivedValue;
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      receivedValue = "";
      for (int i = 0; i < rxValue.length(); i++) {
        receivedValue += rxValue[i];
      }
      Serial.print("Received: ");
      Serial.println(receivedValue);
    }
  }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  // Add BLE2902 descriptor to enable notifications
  BLEDescriptor *pDescr;
  pDescr = new BLEDescriptor(BLEUUID((uint16_t)0x2901));//Creates a new descriptor with the UUID 0x2901, which is the "Characteristic User Description" descriptor.
  pDescr->setValue("Helmet Unit");


  // pCharacteristic->addDescriptor(new BLE2902());
  BLE2902* pBLE2902;
  pBLE2902 = new BLE2902(); //Creates a new BLE2902 descriptor. This is the Client Characteristic Configuration Descriptor (CCCD), required for notifications/indications.
  pBLE2902->setNotifications(true);
  pCharacteristic->addDescriptor(pDescr);
  pCharacteristic->addDescriptor(pBLE2902);
  pCharacteristic->setValue("Hello this is Helmet Unit");
  pCharacteristic->setCallbacks(new MyCallbacks()); 

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        // pCharacteristic->setValue("Hello this is Helmet Unit");
        std::string message = "Hello this is Helmet Unit";
        pCharacteristic->setValue(value);
        Serial.print("Sending Value: ");   
        Serial.println(value);
        pCharacteristic->notify();
        value++;
        delay(1000);
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}