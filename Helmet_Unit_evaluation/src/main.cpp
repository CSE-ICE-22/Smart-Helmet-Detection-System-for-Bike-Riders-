#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

#define FSR_PIN 0 
#define TOUCH_PIN 5        // TTP223 touch sensor → HIGH = touched (helmet worn)
#define BUCKLE_PIN 6       // Buckle switch input (active LOW when buckled)
#define BUTTON_PIN 7      // push button to enable/disable pairing 

BLECharacteristic *txCharacteristic;
BLECharacteristic *rxCharacteristic;
BLEServer *pServer;
BLEAdvertising *pAdvertising;

bool deviceConnected = false;
bool isAdvertising = false;
bool lastButtonState = HIGH;

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Bike connected.");
    isAdvertising = false;
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Bike disconnected.");
    pAdvertising->start();
    isAdvertising = true;
  }
};

class RxCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("Received from Bike: ");
      Serial.println(rxValue.c_str());
      // Example: If "true", could trigger something; unused in original.
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting Helmet Unit...");
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUCKLE_PIN, INPUT_PULLUP);   
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  BLEDevice::init("HelmetUnit");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  txCharacteristic = pService->createCharacteristic(
    CHAR_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY
  );

  rxCharacteristic = pService->createCharacteristic(
    CHAR_UUID_RX, BLECharacteristic::PROPERTY_WRITE
  );
  rxCharacteristic->setCallbacks(new RxCallbacks());

  pService->start();

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  Serial.println("Helmet ready. Press button to enable pairing.");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW && !isAdvertising) {
    Serial.println("Button pressed → start advertising for pairing");
    pAdvertising->start();
    isAdvertising = true;
  }

  if (deviceConnected) {
    unsigned long startTime = millis();  // Start timing

    bool helmetTouched = (digitalRead(TOUCH_PIN) == HIGH);
    int fsrValue = analogRead(FSR_PIN);
    bool buckled = (digitalRead(BUCKLE_PIN) == LOW);

    Serial.printf("helmetTouched: %d, fsrValue: %d, buckled: %d\n", 
                  helmetTouched, fsrValue, buckled);

    String status;
    if (helmetTouched && fsrValue > 50 && buckled) {
      status = "true";
    } else {
      status = "warn";
    }

    txCharacteristic->setValue(status.c_str());
    txCharacteristic->notify();

    unsigned long endTime = millis();  // End timing
    unsigned long responseTime = endTime - startTime;
    Serial.printf("Status sent: %s, Helmet Response Time: %lu ms\n", status.c_str(), responseTime);

    delay(500);  // Adjust for experiment rate
  }
}