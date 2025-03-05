#include <Arduino.h>
#include "HX711.h"
#include <Stepper.h>

// Load Cell (HX711) Pins
const int LOADCELL_DOUT_PIN = 2;  // ESP32 pin for HX711 DOUT
const int LOADCELL_SCK_PIN = 3;   // ESP32 pin for HX711 SCK

// Ultrasonic Sensor Pins
#define TRIGGER_PIN 12    // ESP32 pin for ultrasonic sensor trigger
#define ECHO_PIN 11       // ESP32 pin for ultrasonic sensor echo
#define MAX_DISTANCE 400  // Maximum measurable distance in cm

// Trashcan Specifications
#define TRASHCAN_HEIGHT 34.0  // Height of the trashcan in cm
#define WEIGHT_THRESHOLD 0.4  // Max weight threshold in kg
#define FILL_LEVEL_THRESHOLD 90.0  // Max fill level threshold in %

// Stepper Motor Configuration
#define STEPS_PER_REV 2048  // Steps per full revolution for 28BYJ-48 motor
#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9

HX711 scale;
Stepper stepperMotor(STEPS_PER_REV, IN1, IN3, IN2, IN4);

bool lidClosed = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Smart Trashcan System Initializing...");

  // Initialize Load Cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-83.648);  // Adjust based on calibration
  scale.tare();               // Reset the scale to zero

  // Initialize Ultrasonic Sensor Pins
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize Stepper Motor
  stepperMotor.setSpeed(20);  // Set motor speed to 20 RPM

  Serial.println("System Ready.");
}

void loop() {
  // --- Weight Measurement (Load Cell) ---
  float weight_grams = scale.get_units(10);  // Average of 10 readings
  float weight_kg = weight_grams / 1000.0;   // Convert grams to kg

  // --- Volume Measurement (Ultrasonic) ---
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance_to_trash = duration * 0.034 / 2;  // Convert time to cm

  float trash_level = 0.0;  // Percentage of trashcan filled
  if (distance_to_trash > 0 && distance_to_trash <= MAX_DISTANCE) {
    trash_level = ((TRASHCAN_HEIGHT - distance_to_trash) / TRASHCAN_HEIGHT) * 100;
  }

  // --- Display Results ---
  Serial.print("Weight: ");
  Serial.print(weight_grams, 2);
  Serial.print(" g (");
  Serial.print(weight_kg, 3);
  Serial.print(" kg) | ");

  Serial.print("Trash Level: ");
  if (distance_to_trash == 0 || distance_to_trash > MAX_DISTANCE) {
    Serial.println("Out of range (sensor error)");
  } else {
    Serial.print(trash_level, 1);
    Serial.println("% full");
  }

  // --- Smart Decision Logic ---
  if (trash_level >= FILL_LEVEL_THRESHOLD || weight_kg >= WEIGHT_THRESHOLD) {
    Serial.println("⚠️ Trashcan is FULL! Closing lid...");
    closeLid();
  } else {
    Serial.println("✅ Trashcan is not full.");
    lidClosed = false;  // Reset lid status when not full
  }

  Serial.println("------------------------------------");
  delay(5000);  // Wait before next measurement
}

// --- Function to Close Lid Using Stepper Motor ---
void closeLid() {
  if (!lidClosed) {
    Serial.println("🔄 Moving lid to close position...");
    for (int i = 0; i < 7; i++) {
      stepperMotor.step(-STEPS_PER_REV);  // Rotate forward
      delay(250);
    }

    Serial.println("✅ Lid closed.");
    lidClosed = true;

    // Open lid after delay
    delay(5000);
    Serial.println("🔄 Reopening lid...");
    for (int i = 0; i < 7; i++) {
      stepperMotor.step(STEPS_PER_REV);  // Rotate backward
      delay(250);
    }
    Serial.println("✅ Lid opened.");
  }
}
