#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PyroelectricPIN 23

// Set LCD address (try 0x27 first, change to 0x3F if not working)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  pinMode(PyroelectricPIN, INPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("PIR Monitor");
  delay(2000);
  lcd.clear();
}

void loop() {
  int ReadValue = digitalRead(PyroelectricPIN);

  lcd.clear();

  if (ReadValue) {
    Serial.println("Someone");
    lcd.setCursor(0, 0);
    lcd.print("Motion Detected");
    lcd.setCursor(0, 1);
    lcd.print("Someone here");
  } 
  else {
    Serial.println("No one");
    lcd.setCursor(0, 0);
    lcd.print("No Motion");
    lcd.setCursor(0, 1);
    lcd.print("Area clear");
  }

  delay(500);
}
