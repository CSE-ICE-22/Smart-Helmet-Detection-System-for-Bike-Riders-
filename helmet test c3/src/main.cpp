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
#define BUTTON_PIN 7       // push button to enable/disable pairing 

BLECharacteristic *txCharacteristic;
BLECharacteristic *rxCharacteristic;
BLEServer *pServer;
BLEAdvertising *pAdvertising;

bool deviceConnected = false;
bool isAdvertising = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 300;  // debounce delay in ms

// --- Timing variables ---
unsigned long bootTime = 0;
unsigned long connectTime = 0;
bool connectTimeRecorded = false;

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    connectTime = millis() - bootTime;  // time from boot to connect
    connectTimeRecorded = true;
    Serial.println("✅ Bike connected.");
    Serial.printf("⏱ BLE connection time: %.2f seconds\n", connectTime / 1000.0);
    isAdvertising = false;
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("❌ Bike disconnected.");
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting Helmet Unit...");

  bootTime = millis();  // Record system start time

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

  pService->start();

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  Serial.println("Helmet ready. Press button to start/stop pairing.");
}

void loop() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Detect button press (active LOW) with debounce
  if (currentButtonState == LOW && lastButtonState == HIGH && 
      (millis() - lastDebounceTime) > debounceDelay) {

    lastDebounceTime = millis();

    if (!isAdvertising && !deviceConnected) {
      Serial.println("🔵 Button pressed → Start advertising (pairing enabled)");
      pAdvertising->start();
      isAdvertising = true;
      connectTimeRecorded = false;  // reset timer for new connection
      bootTime = millis();          // reset base time for timing measurement
      delay(500);

    } else if (isAdvertising) {
      Serial.println("🟡 Button pressed → Stop advertising");
      pAdvertising->stop();
      isAdvertising = false;
      delay(500);

    } else if (deviceConnected) {
      Serial.println("🔴 Button pressed → Disconnect BLE device");
      pServer->disconnect(pServer->getConnId());
      deviceConnected = false;
      isAdvertising = false;
      delay(500);
    }
  }

  lastButtonState = currentButtonState;

  // Helmet logic (only active when connected)
  if (deviceConnected) {
    bool helmetTouched = (digitalRead(TOUCH_PIN) == HIGH);
    int fsrValue = analogRead(FSR_PIN);
    bool buckled = (digitalRead(BUCKLE_PIN) == LOW);

    Serial.printf("helmetTouched: %d, fsrValue: %d, buckled: %d\n",
                  helmetTouched, fsrValue, buckled);

    if (helmetTouched && fsrValue > 50 && buckled) {
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
