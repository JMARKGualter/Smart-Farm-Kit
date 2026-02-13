#include <ESP32Servo.h>

Servo myservo;
int servoPin = 26;

// Adjust these values if needed
int STOP = 90;
int OPEN = 180;   // clockwise
int CLOSE = 0;    // counter-clockwise

void setup() {
  Serial.begin(9600);
  myservo.attach(servoPin);

  myservo.write(STOP);
  delay(1000);
}

void loop() {
  // OPEN (rotate one direction)
  myservo.write(OPEN);
  delay(2000);   // rotation time (adjust for full open)

  // STOP
  myservo.write(STOP);
  delay(1000);

  // CLOSE (rotate opposite direction)
  myservo.write(CLOSE);
  delay(2000);   // rotation time (adjust for full close)

  // STOP
  myservo.write(STOP);
  delay(3000);
}
