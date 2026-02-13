#include <LiquidCrystal_I2C.h>

#define PhotocecllPin 34   


LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);

 
  pinMode(PhotocecllPin, INPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Photoresistor");
}

void loop() {
  
  int ReadValue = analogRead(PhotocecllPin);

  Serial.print("Photocell value: ");
  Serial.println(ReadValue);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Light Value:");
  lcd.setCursor(0, 1);
  lcd.print(ReadValue);

  delay(500);
}
