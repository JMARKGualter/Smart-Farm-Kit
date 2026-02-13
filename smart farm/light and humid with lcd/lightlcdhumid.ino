#include <LiquidCrystal_I2C.h>
#include <dht11.h>

#define DHT11PIN 17
#define LED 27

#define MotorPin1 19  // IN+
#define MotorPin2 18  // IN-

dht11 DHT11;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init();
  lcd.backlight();

  pinMode(LED, OUTPUT);
  pinMode(MotorPin1, OUTPUT);
  pinMode(MotorPin2, OUTPUT);

  
  digitalWrite(MotorPin1, LOW);
  digitalWrite(MotorPin2, LOW);
}

void loop() {
  int Temperature;
  int Humidity;

  DHT11.read(DHT11PIN);
  Temperature = DHT11.temperature;
  Humidity = DHT11.humidity;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(Temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(Humidity);
  lcd.print(" %");

  if (Temperature > 30) {
    
    digitalWrite(LED, HIGH);

    
    digitalWrite(MotorPin1, HIGH);
    digitalWrite(MotorPin2, LOW);

    delay(300);
    digitalWrite(LED, LOW);
    delay(300);
  } 
  else {
    digitalWrite(LED, LOW);

    digitalWrite(MotorPin1, LOW);
    digitalWrite(MotorPin2, LOW);

    delay(600);
  }
}
