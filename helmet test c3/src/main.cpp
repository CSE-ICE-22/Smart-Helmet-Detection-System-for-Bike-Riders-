
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
bool isAdvertising = false;   // <--- custom flag
bool lastButtonState = HIGH;

class ServerCallbacks: public BLEServerCallbacks {
/*************  ✨ Windsurf Command ⭐  *************/
  /**
   * Called when a client (Bike) connects to the server.
   * @param pServer The BLEServer object.
   */
/*******  3aeb4de4-53db-45e9-b0f8-4679048743dc  *******/
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Bike connected.");
    isAdvertising = false;  // stop flag when connected
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Bike disconnected.");
    // Restart advertising automatically
    pAdvertising->start();
    isAdvertising = true;
  }
};

void setup() {
 Serial.begin(115200);
  delay(1000); // wait 1 second to stabilize
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

  // Start the service
  pService->start();

  // Create advertiser and add service UUID
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);


  Serial.println("Helmet ready. Press button to enable pairing.");
}

void loop() {
  // Start advertising only if button pressed & not already advertising
  if (digitalRead(BUTTON_PIN) == LOW && !isAdvertising) {
    Serial.println("Button pressed → start advertising for pairing");
    pAdvertising->start();
    isAdvertising = true;
  }

  
  if (deviceConnected) {
    bool helmetTouched = (digitalRead(TOUCH_PIN) == HIGH); // TTP223
    int fsrValue = analogRead(FSR_PIN);                   // FSR
    bool buckled = (digitalRead(BUCKLE_PIN) == LOW);       // active LOW when buckled

    // Debug print
    Serial.printf("helmetTouched: %d, fsrValue: %d, buckled: %d\n", 
                  helmetTouched, fsrValue, buckled);

   // Logic: First helmet touch → then FSR → then buckle
    if (helmetTouched && fsrValue > 500 && buckled) {
      txCharacteristic->setValue("true");
      txCharacteristic->notify();
      Serial.println("✅ Helmet touch + FSR + buckle → Sent TRUE to Bike.");
    } else {
      txCharacteristic->setValue("warn");
      txCharacteristic->notify();
      Serial.println("⚠️ Warning: Missing condition → Sent WARN to Bike.");
    }
    delay(500);
  }
}
