#include <Arduino.h>
#include "HX711.h"
#include <Stepper.h>
#include <Wire.h>
#include "MLX90641_API.h"
#include "MLX9064X_I2C_Driver.h"

// Load Cell (HX711) Pins
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Stepper Motor Pins
#define STEPS_PER_REV 2048
#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9

// Ultrasonic Sensor Pins
const int TRIG_PIN_HC = 5; // HC-SR04 facing downward
const int ECHO_PIN_HC = 4;
const int TRIG_PIN_JSN = 11; // JSN-SR04T facing across the top
const int ECHO_PIN_JSN = 10;

// Temperature Sensor (MLX90641)
const byte MLX90641_address = 0x33;
#define TA_SHIFT 8

// Trashcan Specifications
#define WEIGHT_THRESHOLD .4  // Max weight threshold in kg
#define TEMP_THRESHOLD 40.0  // Temperature threshold in Celsius
#define TRASH_FULL_DISTANCE 10  // Distance threshold in cm for HC-SR04
#define OBJECT_DETECTED_DISTANCE 25  // Distance threshold in cm for JSN-SR04T

HX711 scale;
Stepper stepperMotor(STEPS_PER_REV, IN1, IN3, IN2, IN4);

uint16_t eeMLX90641[832];
float MLX90641To[192];
uint16_t MLX90641Frame[242];
paramsMLX90641 MLX90641;

void setup() {
    Serial.begin(115200);
    Serial.println("Smart Trashcan System Initializing...");

    // Initialize Load Cell
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(-83.648);
    scale.tare();

    // Initialize Stepper Motor
    stepperMotor.setSpeed(20);

    // Initialize Temperature Sensor
    Wire.begin();
    Wire.setClock(400000);
    
    if (!isConnected()) {
        Serial.println("MLX90641 not detected. Please check wiring.");
        while (1);
    }

    int status = MLX90641_DumpEE(MLX90641_address, eeMLX90641);
    if (status != 0) {
        Serial.println("Failed to load system parameters");
        while (1);
    }

    status = MLX90641_ExtractParameters(eeMLX90641, &MLX90641);
    if (status != 0) {
        Serial.println("Parameter extraction failed");
        while (1);
    }

    MLX90641_SetRefreshRate(MLX90641_address, 0x05);

    // Initialize Ultrasonic Sensors
    pinMode(TRIG_PIN_HC, OUTPUT);
    pinMode(ECHO_PIN_HC, INPUT);
    pinMode(TRIG_PIN_JSN, OUTPUT);
    pinMode(ECHO_PIN_JSN, INPUT);

    Serial.println("System Ready.");
}

void loop() {
    // --- Weight Measurement ---
    float weight_grams = scale.get_units(10);
    float weight_kg = weight_grams / 1000.0;

    // --- Temperature Measurement ---
    int status = MLX90641_GetFrameData(MLX90641_address, MLX90641Frame);
    float Ta = MLX90641_GetTa(MLX90641Frame, &MLX90641);
    float tr = Ta - TA_SHIFT;
    float emissivity = 0.95;
    MLX90641_CalculateTo(MLX90641Frame, &MLX90641, emissivity, tr, MLX90641To);

    float maxTemp = 0.0;
    for (int x = 0; x < 192; x++) {
        if (MLX90641To[x] > maxTemp) {
            maxTemp = MLX90641To[x];
        }
    }

   // **NEW: Only print the 192 temperature values in CSV format**
    for (int x = 0; x < 192; x++) {
        Serial.print(MLX90641To[x], 2);
        if (x < 191) Serial.print(",");  // Add comma between values
    }
    Serial.println("");  // End the line for Python to read

    // --- Distance Measurement ---
    float distanceHC = getDistance(TRIG_PIN_HC, ECHO_PIN_HC);
    float distanceJSN = getDistance(TRIG_PIN_JSN, ECHO_PIN_JSN);

    // Serial.print("Weight: ");
    // Serial.print(weight_grams, 2);
    // Serial.print(" g ( ");
    // Serial.print(weight_kg, 3);
    // Serial.print(" kg ) | Max Temperature: ");
    // Serial.print(maxTemp, 2);
    // Serial.print(" °C | HC-SR04: ");
    // Serial.print(distanceHC);
    // Serial.print(" cm | JSN-SR04T: ");
    // Serial.print(distanceJSN);
    // Serial.println(" cm");

    // --- Lid Closing Logic ---
    if (weight_kg >= WEIGHT_THRESHOLD || maxTemp >= TEMP_THRESHOLD || 
        (distanceHC <= TRASH_FULL_DISTANCE && distanceJSN <= OBJECT_DETECTED_DISTANCE)) {
        //Serial.println("⚠️ Closing Trashcan Lid!");
        closeLid();
    } else {
        //Serial.println("✅ Trashcan is not full.");
    }

    //Serial.println("------------------------------------");
    delay(250);
}

void closeLid() {
    for (int i = 0; i < 6; i++) {
        stepperMotor.step(-STEPS_PER_REV);
    }
    for (int i = 0; i < 5; i++) {
        stepperMotor.step(STEPS_PER_REV);
    }
}

float getDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;
    return distance;
}

boolean isConnected() {
    Wire.beginTransmission((uint8_t)MLX90641_address);
    return (Wire.endTransmission() == 0);
}
