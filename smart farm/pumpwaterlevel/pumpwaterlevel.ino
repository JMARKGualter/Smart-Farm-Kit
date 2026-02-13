#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RelayPin 25
#define WaterLevelPin 33

int threshold = 1000;

// LCD address: try 0x27, kung walang display, palitan ng 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);

  pinMode(RelayPin, OUTPUT);
  pinMode(WaterLevelPin, INPUT);

  digitalWrite(RelayPin, LOW); // pump OFF muna

  // LCD init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Water Monitor");
  lcd.setCursor(0,1);
  lcd.print("Starting...");
  lcd.clear();
}

void loop() {
  int ReadValue = analogRead(WaterLevelPin);

  Serial.print("Water Level: ");
  Serial.println(ReadValue);

  // ===== LCD LINE 1 =====
  lcd.setCursor(0,0);
  lcd.print("Level:");
  lcd.print(ReadValue);
  lcd.print("    "); // clear extra chars

  // ===== CHECK LEVEL =====
  if (ReadValue > threshold) {
    digitalWrite(RelayPin, HIGH);   // pump ON

    Serial.println("Pump ON");

    lcd.setCursor(0,1);
    lcd.print("Pump: ON  HIGH ");
  } 
  else {
    digitalWrite(RelayPin, LOW);    // pump OFF

    Serial.println("Pump OFF");

    lcd.setCursor(0,1);
    lcd.print("Pump: OFF LOW  ");
  }

 
}
