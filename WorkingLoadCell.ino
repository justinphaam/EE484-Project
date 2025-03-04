#include <Arduino.h>
#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

HX711 scale;

void setup() {
  Serial.begin(57600);
  Serial.println("HX711 Demo");
  Serial.println("Initializing the scale");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());      // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));   // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
                                          // by the SCALE parameter (not set yet)
            
  scale.set_scale(-105.313);  // Set scale factor (calibrated with known weight)
  scale.tare();               // Reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());                 // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
                                                // by the SCALE parameter set with set_scale

  Serial.println("Readings:");
}

void loop() {
  float weight_grams = scale.get_units();      // Get a single weight reading in grams
  float weight_grams_avg = scale.get_units(10); // Get the average of 10 readings in grams
  float weight_kg = weight_grams / 1000.0;     // Convert to kilograms
  float weight_kg_avg = weight_grams_avg / 1000.0; // Convert average weight to kg

  Serial.print("Weight: ");
  Serial.print(weight_grams, 2);  // Print single reading in grams
  Serial.print(" g \t| ");
  Serial.print(weight_kg, 3);  // Print single reading in kg
  Serial.print(" kg \t| ");

  Serial.print("Average: ");
  Serial.print(weight_grams_avg, 2);  // Print average in grams
  Serial.print(" g \t| ");
  Serial.print(weight_kg_avg, 3);  // Print average in kg
  Serial.println(" kg");

  delay(5000);
}
