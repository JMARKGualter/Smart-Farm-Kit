#include <LiquidCrystal_I2C.h>
#include <dht11.h>

#define DHT11PIN 17
#define LED 27   // LED pin

dht11 DHT11;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init();
  lcd.backlight();

  pinMode(LED, OUTPUT);
}

void loop() {
  int Temperature;
  int Humidity;

  // Read DHT11
  int chk = DHT11.read(DHT11PIN);

  Temperature = DHT11.temperature;
  Humidity = DHT11.humidity;

  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(Temperature);

  lcd.setCursor(0, 1);
  lcd.print("Hum:");
  lcd.print(Humidity);

  // ✅ Condition: If temperature > 20 → LED blink
  if (Temperature > 20) {
    digitalWrite(LED, HIGH);
    delay(300);
    digitalWrite(LED, LOW);
    delay(300);
  } 
  else {
    digitalWrite(LED, LOW);
    delay(600);
  }
}
