#include <Stepper.h>

#define STEPS_PER_REV 2048  // Full steps per revolution for 28BYJ-48

// Define stepper motor control pins (ULN2003 driver)
#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9

Stepper stepperMotor(STEPS_PER_REV, IN1, IN3, IN2, IN4);

int numRotations = 6; // Set the number of rotations
int currentRotation = 0; // Track the completed rotations
int numRotations1 = 6; // Set the number of rotations
int currentRotation1 = 0;
bool done= false;
void setup() {
    Serial.begin(9600);
    stepperMotor.setSpeed(20);  // 20 RPM
    Serial.println("Stepper Motor Test Starting...");
}

void loop() {
    if (currentRotation < numRotations) {
        Serial.print("Moving forward... Rotation: ");
        Serial.println(currentRotation + 1);

        stepperMotor.step(-STEPS_PER_REV);  // Move 1 full revolution forward
        currentRotation++; // Increment rotation count
        delay(250); // Small delay between rotations
    } else { done=true;
        Serial.println("Target rotations reached. Stopping motor.");
        
    }
    if (currentRotation1 < numRotations1&&done) { 
        Serial.print("Moving forward... Rotation: ");
        Serial.println(currentRotation1 + 1);

        stepperMotor.step(STEPS_PER_REV);  // Move 1 full revolution forward
        currentRotation1++; // Increment rotation count
        delay(250); // Small delay between rotations
    } else {
        Serial.println("Target rotations reached. Stopping motor.");
       
    }
}