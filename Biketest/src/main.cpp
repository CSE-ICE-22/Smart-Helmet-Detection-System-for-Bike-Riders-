#include <Arduino.h>
#include <BLEDevice.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// #include "esp_sleep.h"
#include "driver/rtc_io.h"

// I2C LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- PIN DEFINITIONS ---
// STAND_PIN: LOW = Stand UP (1), HIGH = Stand DOWN (0). Uses INPUT_PULLUP.
#define STAND_PIN 26    
// RIDING_PIN: LOW = Riding (1), HIGH = Stationary (0). Uses INPUT_PULLUP.
#define RIDING_PIN 34   
// IGNITION_PIN: Output pin to control the ignition relay (HIGH = ON, LOW = OFF)
#define IGNITION_PIN 25 
// BUZZER_PIN: Output pin for the warning buzzer (HIGH = ON, LOW = OFF)
#define BUZZER_PIN 27   

#define BLE_red 2
#define BLE_green 4

// Assuming Starter switch is HIGH when ON (to simulate a wake-up event)
#define STARTER_WAKEUP_PIN 32 // Choose an RTC GPIO pin (e.g., GPIO 32)
#define WAKEUP_TRIGGER_LEVEL 1 // Wake up on HIGH (Starter ON)

// --- BLE SERVICE DEFINITIONS (Must match the Helmet Unit) ---
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Receive notifications from Helmet (Helmet Status)
#define CHAR_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e" // Send commands to Helmet (Stand Status) - NOT USED

// --- BLE Globals ---
static BLERemoteCharacteristic* txCharacteristic;
static BLERemoteCharacteristic* rxCharacteristic;
static BLEAdvertisedDevice* myDevice;

// --- System State Variables ---
bool connected = false;
bool doConnect = false;
bool doScan = true; 
unsigned long lastScanStartTime = 0;
const long scanDuration = 5000; // Scan for 5 seconds

// Safety System State
bool helmetSecure = false;     // 1=Secure ("true"), 0=Warning ("warn")
bool helmetworn = false;       // Helmet worn state
bool ignitionEnabled = false;  // Tracks the current intended ignition state (HIGH/LOW on IGNITION_PIN)

// Warning Timers (Non-blocking using millis())
unsigned long warningStartTime = 0; 
const long warningDuration15s = 15000; // 15 seconds
const long warningDuration60s = 60000; // 1 minute

// BLE Disconnect Grace Period Timer
unsigned long disconnectStartTime = 0;
const long disconnectGracePeriod = 60000; // 60 seconds for BLE=0 shutdown

// --- HIBERNATION/Deep Sleep Timer ---
unsigned long starterOffTime = 0;
const long hibernationDelayMs = 80000; // 80 seconds before going to sleep

// ---------------- Wakeup reason ----------------
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by Starter (EXT0)");
      break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      Serial.println("Normal power-on boot");
      break;
    default:
      Serial.printf("Wakeup from other source: %d\n", cause);
      break;
  }
}

// ---------------- Deep Sleep Function ----------------
void enterDeepSleep() {
  Serial.println("🛑 Entering deep sleep...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Deep Sleep Mode");
  lcd.setCursor(0, 1);
  lcd.print("Wake: Starter Btn");

  delay(1000); // Let LCD show message before power off
  lcd.noBacklight(); // Turn off LCD backlight before sleep
  lcd.clear();

  // Turn off outputs to save power
  digitalWrite(IGNITION_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(BLE_red, LOW);
  digitalWrite(BLE_green, LOW);

  // Enable external wake-up (Starter pin HIGH)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)STARTER_WAKEUP_PIN, WAKEUP_TRIGGER_LEVEL);

  // Isolate unused RTC GPIOs to reduce leakage (optional)
  rtc_gpio_isolate(GPIO_NUM_12);
  rtc_gpio_isolate(GPIO_NUM_13);
  rtc_gpio_isolate(GPIO_NUM_14);
  rtc_gpio_isolate(GPIO_NUM_15);

  // Enter sleep (won’t return until wake-up)
  esp_deep_sleep_start();
}

// ---------------- BLE Client Callback ----------------
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) override {
    connected = true;
    disconnectStartTime = 0; // Clear grace timer immediately on successful re-connection
    Serial.println("✅ Connected to Helmet.");
    digitalWrite(BLE_green, HIGH);
    digitalWrite(BLE_red, LOW);
  }

  void onDisconnect(BLEClient* pClient) override {
    connected = false;
    Serial.println("⚠️ Disconnected from Helmet. Starting 60s grace period.");
    digitalWrite(BLE_green, LOW); 
    digitalWrite(BLE_red, HIGH);
    // Only start the shutdown timer if the ignition was enabled when disconnected
    if (ignitionEnabled) { 
      disconnectStartTime = millis();
    } else {
      // If ignition was already off, enforce full shutdown immediately (no need for timer)
      disconnectStartTime = 0; 
      digitalWrite(IGNITION_PIN, LOW); 
      digitalWrite(BUZZER_PIN, LOW);
      helmetSecure = false; // Assume insecure when disconnected
      helmetworn = false;
    }

    doScan = true;  // Trigger scan again
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("Disconnected!");
  }
};

// ---------------- BLE Advertisement Callback ----------------
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (advertisedDevice.haveServiceUUID() && 
        advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.println("✅ Helmet found! Stopping scan.");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      lcd.clear();
      lcd.setCursor(0, 0); 
      lcd.print("Helmet found!");
    }
  }
};

// ---------------- Notification Callback (Helmet Status) ----------------
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData, size_t length, bool isNotify
) {
  String msg = "";
  for (size_t i = 0; i < length; i++) msg += (char)pData[i];

  // Update the global helmet state
  if (msg == "true") {
    helmetSecure = true; // 1: Worn & Buckled
    helmetworn = true;
    Serial.println("✅ Helmet secure: Worn & Buckled");
  } else if (msg == "warn") {
    helmetSecure = false; // 0: Not Worn or Buckle Open
    helmetworn = false;
    Serial.println("⚠️ Warning: Helmet not worn and buckle open!");
  }else if (msg == "warn_notbuckeld") {
    helmetSecure = false; // 0: Not Worn or Buckle Open
    helmetworn = true;
    Serial.println("⚠️ Warning: Helmet worn but buckle open!");
  } else {
    helmetSecure = false; // Default to insecure
    helmetworn = false;
  }
}

// ---------------- Connect to Helmet ----------------
bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  if (!pClient->connect(myDevice)) return false;

  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) return false;

  txCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHAR_UUID_TX));
  rxCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHAR_UUID_RX));

  if (txCharacteristic == nullptr || rxCharacteristic == nullptr) return false;

  if (txCharacteristic->canNotify()) {
    txCharacteristic->registerForNotify(notifyCallback);
  }

  // Connection successful, clear any lingering disconnect timer
  connected = true;
  disconnectStartTime = 0; 
  return true;
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL for ESP32

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bike Unit Booting");

  // Check and print the wake-up reason
  print_wakeup_reason();
  
  pinMode(BLE_red, OUTPUT);
  pinMode(BLE_green, OUTPUT);
  digitalWrite(BLE_red, HIGH);
  digitalWrite(BLE_green, LOW);

  // Initialize Input Pins with internal pull-up resistors
  pinMode(STAND_PIN, INPUT_PULLUP); 
  pinMode(RIDING_PIN, INPUT_PULLUP);

  // Initialize Output Pins
  pinMode(IGNITION_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initial State: Ignition and Buzzer OFF (Disabled)
  digitalWrite(IGNITION_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  ignitionEnabled = false;


  // --- RTC GPIO configuration for Starter wake pin ---
  rtc_gpio_init((gpio_num_t)STARTER_WAKEUP_PIN);
  rtc_gpio_set_direction((gpio_num_t)STARTER_WAKEUP_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_en((gpio_num_t)STARTER_WAKEUP_PIN);
  rtc_gpio_pullup_dis((gpio_num_t)STARTER_WAKEUP_PIN);

  BLEDevice::init("BikeUnit");

  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);

  // **CRITICAL INITIAL CHECK**
  // If the starter is ON at boot, we clear the hibernation timer right away.
  if (digitalRead(STARTER_WAKEUP_PIN) == HIGH) {
      starterOffTime = 0;
      Serial.println("Starter ON at boot. Hibernation timer cleared.");
  }
}

// ---------------- Main Loop ----------------
void loop() {
   bool isStarterOn = (digitalRead(STARTER_WAKEUP_PIN) == HIGH);

  // --- Deep Sleep Trigger ---
  if (!isStarterOn) {
    if (starterOffTime == 0) {
      starterOffTime = millis();
      Serial.println("Starter OFF → starting 80s timer");
    } else if (millis() - starterOffTime >= hibernationDelayMs) {
      enterDeepSleep();
    }
  } else {
    starterOffTime = 0; // Reset timer when starter ON
  }
  // --- A. Connection Management ---
  if (doConnect) {
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("Connecting...");

    if (connectToServer()) {
      Serial.println("✅ Successfully connected to Helmet.");
    } else {
      Serial.println("❌ Connection failed. Will rescan...");
      lcd.clear();
      lcd.setCursor(0, 0); 
      lcd.print("Connection FAILED");
      doScan = true; // Re-enable scan
    }
    doConnect = false;
  }

  if (doScan && !connected) {
    if (millis() - lastScanStartTime >= scanDuration || lastScanStartTime == 0) {
      if (disconnectStartTime == 0) { // Only show Scanning if not in grace period
        Serial.println("🔍 Scanning for Helmet...");
        lcd.setCursor(0, 0); 
        lcd.print("Scanning...     "); 
      }
      BLEDevice::getScan()->start(scanDuration / 1000.0, false);
      lastScanStartTime = millis();
    }
  }

  // --- B. BLE Disconnection Grace Period Logic (BLE = 0) ---
  if (!connected && disconnectStartTime > 0) {
    // If we are disconnected AND the ignition was ON, we are in the 60s grace period.
    unsigned long elapsed = millis() - disconnectStartTime;

    if (elapsed >= disconnectGracePeriod) {
      // Grace period expired: force OFF
      digitalWrite(IGNITION_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      ignitionEnabled = false;
      disconnectStartTime = 0;
      Serial.println("❌ 60s BLE GRACE PERIOD EXPIRED. IGNITION DISABLED.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BLE Shutdown!");
      lcd.setCursor(0, 1);
      lcd.print("Ignition OFF");
    } else {
      // Still within grace period: maintain current IGNITION_PIN state
      // LCD Update for disconnected state
      lcd.setCursor(0, 0);
      lcd.print("BLE Disconnected");
      lcd.setCursor(0, 1);
      long remaining = (disconnectGracePeriod - elapsed) / 1000;
      lcd.print("Shutdown in ");
      lcd.print(remaining);
      lcd.print("s ");
    }
    
    // Skip main safety logic when disconnected and in grace period
    delay(500);
    return; 
  }

  // --- C. Safety Logic (Only runs if connected is true, or if grace period just ended) ---
  if (connected) {
    
    // 1. Read Inputs (Stand: 1=UP, 0=DOWN | Riding: 1=RIDING, 0=STATIONARY)
    bool isStandUp = (digitalRead(STAND_PIN) == LOW);
    bool isRiding = (digitalRead(RIDING_PIN) == LOW); 
    
    // 2. Core Logic Evaluation based on Truth Table
    // Ignition ON: (S=1 AND H=1)
    bool requiredIgnition = isStandUp && helmetSecure;

    // 60s Warning: (S=0, R=0, H=1) - Stand Down, Stationary, Helmet Secure
    bool require60sWarning = (!isStandUp && !isRiding && helmetworn);
    
    // 15s Warning: (S=0, R=1, H=0 or 1) OR (S=1, R=1, H=0)
    bool require15sWarning = (!isStandUp && isRiding) || (isStandUp && isRiding && !helmetSecure);

    // --- State Machine Implementation ---

    if (requiredIgnition) {
        // Case: Ignition ON (S=1 AND H=1)
        if (!ignitionEnabled) {
            ignitionEnabled = true;
            digitalWrite(IGNITION_PIN, HIGH);
            Serial.println("🔥 IGNITION ENABLED.");
        }
        // Clear all warnings and buzzer when ignition is ON
        warningStartTime = 0;
        digitalWrite(BUZZER_PIN, LOW);
    } 
    else { 
        // Case: Ignition OFF (S=0 OR H=0)
        
        // 1. Check for 60-second warning condition (Immediate Ignition Cut, Buzzer for 60s)
        if (require60sWarning) {
            if (warningStartTime == 0) {
                ignitionEnabled = false;
                digitalWrite(IGNITION_PIN, LOW); // **IMMEDIATE IGNITION CUT**
                warningStartTime = millis();
                Serial.println("🔔 Starting 60s warning (Stand Down, Stationary, Helmet Secure). IGNITION CUT.");
            }
            digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON for the duration

            if (millis() - warningStartTime >= warningDuration60s) {
                // Buzzer stops after 60 seconds (Ignition is already off)
                digitalWrite(BUZZER_PIN, LOW);
                warningStartTime = 0;
                Serial.println("❌ 60s WARNING EXPIRED. BUZZER DISABLED.");
            }
        }
        
        // 2. Check for 15-second warning condition
        else if (require15sWarning) {
            if (warningStartTime == 0) {
                // ignitionEnabled = false; // Ensure ignition is off on entering warning
                // digitalWrite(IGNITION_PIN, LOW);
                warningStartTime = millis();
                Serial.println("🔔 Starting 15s warning (Hazard detected). ");
            }
            // digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON

            if (millis() - warningStartTime >= warningDuration15s) {
                // // Buzzer stops after 15 seconds (Ignition is already off)
                // digitalWrite(BUZZER_PIN, LOW);
                ignitionEnabled = false; // Ensure ignition is off after warning
                digitalWrite(IGNITION_PIN, LOW);
                warningStartTime = 0;
                // Serial.println("❌ 15s WARNING EXPIRED. BUZZER DISABLED.");
                Serial.println("❌ 15s WARNING EXPIRED. IGNITION CUT.");

            }
        }
        
        // 3. Default Disabled State (All other cases where ignition is not required)
        else { 
            ignitionEnabled = false;
            digitalWrite(IGNITION_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
            warningStartTime = 0;
        }
    }

    // --- D. LCD Status Update (Connected State) ---
    lcd.setCursor(0, 0);
    lcd.print("S:");
    lcd.print(isStandUp ? "UP " : "DN ");
    lcd.print(" R:");
    lcd.print(isRiding ? "ON " : "OFF");
    lcd.print(" I:");
    lcd.print(ignitionEnabled ? "ON " : "OFF");

    lcd.setCursor(0, 1);
    
    if (warningStartTime > 0) {
        // Show remaining time
        unsigned long elapsed = millis() - warningStartTime;
        long remainingDuration = require60sWarning ? warningDuration60s : warningDuration15s;
        long remaining = (remainingDuration - elapsed) / 1000;
        if (remaining < 0) remaining = 0;
        
        lcd.print("WARNING: ");
        lcd.print(remaining);
        lcd.print("s ");

    } else {
        // Show Helmet status
        lcd.print("H: ");
        lcd.print(helmetSecure ? "SECURE " : "WARN   ");
        lcd.print("       "); 
    }
    
    // Slow down the loop execution
    delay(500); 
  } else {
    // If not connected and not in grace period, run slowly for scanning
    if (disconnectStartTime == 0) {
        delay(100); 
    }
  }
}
