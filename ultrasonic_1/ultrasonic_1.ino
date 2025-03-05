#include <NewPing.h>

#define TRIGGER_PIN 12    // Arduino pin tied to trigger pin on the ultrasonic sensor
#define ECHO_PIN 11       // Arduino pin tied to echo pin on the ultrasonic sensor
#define MAX_DISTANCE 200  // Maximum distance to ping for (in cm)

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);  // NewPing setup

void setup() {
  Serial.begin(115200);  // Open Serial Monitor at 115200 baud
}

void loop() {
  delay(50);  // Wait 50ms between pings (about 20 pings/sec)
  Serial.print("Ping: ");
  Serial.print(sonar.ping_cm());  // Get distance in cm and print
  Serial.println(" cm");
}
