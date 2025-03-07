#include <Arduino.h>
#include "HX711.h"
#include <Stepper.h>
#include <Wire.h>
#include "MLX90641_API.h"
#include "MLX9064X_I2C_Driver.h"

// Load Cell (HX711) Pins
const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 3;

// Stepper Motor Pins
#define STEPS_PER_REV 2048
#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9

// Ultrasonic Sensor Pins
const int TRIG_PIN_HC1 = 5;  // First HC-SR04 facing downward
const int ECHO_PIN_HC1 = 4;
const int TRIG_PIN_HC2 = 11; // Second HC-SR04 facing downward
const int ECHO_PIN_HC2 = 10;

// Button Pin
const int BUTTON_PIN = 2; // Adjust based on wiring

// Temperature Sensor (MLX90641)
const byte MLX90641_address = 0x33;
#define TA_SHIFT 8

// Trashcan Specifications
#define WEIGHT_THRESHOLD 1.5  // Max weight threshold in kg
#define TEMP_THRESHOLD 40.0   // Temperature threshold in Celsius
#define TRASH_FULL_DISTANCE 10  // Distance threshold in cm for both HC-SR04 sensors

HX711 scale;
Stepper stepperMotor(STEPS_PER_REV, IN1, IN3, IN2, IN4);

uint16_t eeMLX90641[832];
float MLX90641To[192];
uint16_t MLX90641Frame[242];
paramsMLX90641 MLX90641;

volatile bool buttonPressed = false; // Flag to indicate button press
bool isLidOpen = true; // Track whether the lid is open or closed

// Interrupt Service Routine (ISR) Prototype
// void IRAM_ATTR buttonPress();
// Interrupt Service Routine (ISR)
void IRAM_ATTR buttonPress() {
    buttonPressed = true;
}

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
    } else Serial.println("MLX90641 detected");

    int status = MLX90641_DumpEE(MLX90641_address, eeMLX90641);
    if (status != 0) {
        Serial.println("Failed to load system parameters");
        while (1);
    } else Serial.println("MLX90641 param loaded");

    status = MLX90641_ExtractParameters(eeMLX90641, &MLX90641);
    if (status != 0) {
        Serial.println("Parameter extraction failed");
        while (1);
    } else Serial.println("MLX90641 param extracted");

    MLX90641_SetRefreshRate(MLX90641_address, 0x04);

    // Initialize Ultrasonic Sensors
    pinMode(TRIG_PIN_HC1, OUTPUT);
    pinMode(ECHO_PIN_HC1, INPUT);
    pinMode(TRIG_PIN_HC2, OUTPUT);
    pinMode(ECHO_PIN_HC2, INPUT);

    // Initialize Button with Interrupt
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Internal pull-up resistor
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPress, FALLING);
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
    float distanceHC1 = getDistance(TRIG_PIN_HC1, ECHO_PIN_HC1);
    float distanceHC2 = getDistance(TRIG_PIN_HC2, ECHO_PIN_HC2);

    // Serial.print("Distance HC1: ");
    // Serial.print(distanceHC1);
    // Serial.print(" cm, Distance HC2: ");
    // Serial.print(distanceHC2);
    // Serial.println(" cm");

    // If the button was pressed, toggle the lid state
    if (buttonPressed) {
        if (isLidOpen) {
            closeLid();
            isLidOpen = false;
        } else {
            openLid();
            isLidOpen = true;
            delay(5000);
        }
        buttonPressed = false;
    }
    // If the weight or temperature threshold is met, or both sensors detect full trash, close the lid
    else if (weight_kg >= WEIGHT_THRESHOLD || maxTemp >= TEMP_THRESHOLD || 
             (distanceHC1 <= TRASH_FULL_DISTANCE && distanceHC2 <= TRASH_FULL_DISTANCE)) {
        if (isLidOpen){
          closeLid();
        }
        isLidOpen = false;
    }
    delay(250);
}

void closeLid() {
    //Serial.println("⚠️ Closing Trashcan Lid!");
    for (int i = 0; i < 6; i++) {
        stepperMotor.step(-STEPS_PER_REV);
    }
}

void openLid() {
    //Serial.println("🔘 Opening Trashcan Lid!");
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
