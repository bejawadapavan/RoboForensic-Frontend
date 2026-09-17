/*
 * ======================================================================================
 * ROBOFORENSIC BLE ROVER FIRMWARE
 * ======================================================================================
 * Device: ESP32 / ESP32-WROOM / ESP32-CAM
 * Bluetooth: BLE (Bluetooth Low Energy)
 * Web Controller: https://bejawadapavan.github.io/RoboForensic-Frontend/bluetooth.html
 *
 * UUIDs:
 *   Service UUID:        4fafc201-1fb5-459e-8fcc-c5c9c331914b
 *   Characteristic UUID: beb5483e-36e1-4688-b7f5-ea07361b26a8
 * ======================================================================================
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DEVICE_NAME         "RoboForensic-Rover"

// ==========================================
// MOTOR PIN DEFINITIONS (L298N / TB6612 / L9110S)
// Adjust these pins according to your hardware wiring!
// ==========================================
#define MOTOR_A_IN1  16  // Left Motor Forward
#define MOTOR_A_IN2  17  // Left Motor Backward
#define MOTOR_B_IN1  18  // Right Motor Forward
#define MOTOR_B_IN2  19  // Right Motor Backward
#define STATUS_LED    2   // ESP32 Onboard Blue LED (GPIO 2 or 33 on ESP32-CAM)

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Function declarations
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();

// ==========================================
// BLE SERVER CALLBACKS
// ==========================================
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      digitalWrite(STATUS_LED, HIGH);
      Serial.println("[BLE] Central device connected!");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      digitalWrite(STATUS_LED, LOW);
      stopMotors();
      Serial.println("[BLE] Device disconnected. Restarting BLE advertising...");
      
      // CRITICAL FIX: Restart advertising so phone can reconnect immediately
      pServer->startAdvertising();
      Serial.println("[BLE] Advertising restarted. Ready for new connections.");
    }
};

// ==========================================
// BLE CHARACTERISTIC CALLBACKS
// ==========================================
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        char cmd = rxValue[0];
        Serial.print("[BLE CMD] Received: ");
        Serial.println(cmd);

        switch (cmd) {
          case 'F':
            Serial.println(" -> Action: FORWARD");
            moveForward();
            break;
          case 'B':
            Serial.println(" -> Action: BACKWARD");
            moveBackward();
            break;
          case 'L':
            Serial.println(" -> Action: LEFT");
            turnLeft();
            break;
          case 'R':
            Serial.println(" -> Action: RIGHT");
            turnRight();
            break;
          case 'S':
            Serial.println(" -> Action: STOP");
            stopMotors();
            break;
          default:
            Serial.print(" -> Action: UNKNOWN (");
            Serial.print(cmd);
            Serial.println(")");
            break;
        }
      }
    }
};

// ==========================================
// MOTOR CONTROL LOGIC
// ==========================================
void moveForward() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
}

void moveBackward() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
}

void turnLeft() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
}

void turnRight() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
}

void stopMotors() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
}

// ==========================================
// SETUP & INITIALIZATION
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==========================================");
  Serial.println("ROBOFORENSIC ESP32 BLE ROVER STARTING...");
  Serial.println("==========================================");

  // Initialize motor pins as outputs
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  // Ensure motors start stopped
  stopMotors();
  digitalWrite(STATUS_LED, LOW);

  // Initialize BLE Device
  BLEDevice::init(DEVICE_NAME);

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic with Read, Write, Write-No-Response, and Notify
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_WRITE_NR |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Configure BLE Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // iPhone connection helper
  pAdvertising->setMinPreferred(0x12);
  
  BLEDevice::startAdvertising();

  Serial.print("BLE Rover Advertising as: ");
  Serial.println(DEVICE_NAME);
  Serial.print("Service UUID: ");
  Serial.println(SERVICE_UUID);
  Serial.println("Waiting for phone to connect...\n");
}

void loop() {
  // Disconnect / reconnect state handling
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give Bluetooth stack time to settle
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(20);
}
