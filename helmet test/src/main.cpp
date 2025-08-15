
#include <Arduino.h> 
// Helmet Unit ESP32 BLE Server
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_TX        "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Helmet → Bike
#define CHAR_UUID_RX        "6e400002-b5a3-f393-e0a9-e50e24dcca9e" // Bike → Helmet

#define FSR_PIN 34   // FSR402 analog input
#define TOUCH_PIN 4  // TTP223 digital input

BLECharacteristic *txCharacteristic;
BLECharacteristic *rxCharacteristic;

bool deviceConnected = false;
String lastResponse = "";

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

class RXCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (!value.empty()) {
            lastResponse = value.c_str();
            Serial.println("Received from Bike: " + lastResponse);
        }
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(TOUCH_PIN, INPUT);

    BLEDevice::init("HelmetUnit");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    txCharacteristic = pService->createCharacteristic(
        CHAR_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );

    rxCharacteristic = pService->createCharacteristic(
        CHAR_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );
    rxCharacteristic->setCallbacks(new RXCallbacks());

    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("Helmet BLE Server started.");
}

void loop() {
    if (deviceConnected) {
        int fsrValue = analogRead(FSR_PIN);
        int touchValue = digitalRead(TOUCH_PIN);

        if (fsrValue > 500 && touchValue == HIGH) {
            Serial.println("Human detected, sending TRUE");
            txCharacteristic->setValue("true");
            txCharacteristic->notify();
        }
        delay(1000);
    }
}
