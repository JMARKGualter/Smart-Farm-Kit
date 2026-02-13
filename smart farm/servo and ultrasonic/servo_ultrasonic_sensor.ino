#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>


Servo myservo;
int servoPin = 26;

int STOP = 90;
int OPEN = 170;   
int CLOSE = 0;    


#define Trigpin 12
#define Echopin 13

LiquidCrystal_I2C lcd(0x27, 16, 2);

long duration;
int distance; 

void setup() {
  Serial.begin(9600);


  myservo.attach(servoPin);
  myservo.write(STOP);


  pinMode(Trigpin, OUTPUT);
  pinMode(Echopin, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Ultrasonic");
}

void loop() {
  
  digitalWrite(Trigpin, LOW);
  delayMicroseconds(2);
  digitalWrite(Trigpin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigpin, LOW);

  duration = pulseIn(Echopin, HIGH, 30000); // timeout added
  distance = duration / 58; 

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  lcd.setCursor(0, 1);
  lcd.print(distance);
  lcd.print(" cm");


  if (distance > 0 && distance < 500) { 
    myservo.write(OPEN);   
  } 
  else {
    myservo.write(STOP);   
  }

  delay(500);
}
