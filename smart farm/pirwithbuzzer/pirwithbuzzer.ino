#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PyroelectricPIN 23
#define BUZZERPIN 16

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---- short alert melody ----
int melody[] = {262, 330, 392, 523};
int noteDurations[] = {8, 8, 8, 4};
int notes = 4;

// detection hold (makes it feel wider / more sensitive)
unsigned long lastDetectTime = 0;
#define HOLD_TIME 4000   // 👈 keep detected state for 4 sec

void playAlert() {
  for (int i = 0; i < notes; i++) {
    int noteDuration = 1000 / noteDurations[i];
    ledcWriteTone(BUZZERPIN, melody[i]);
    delay(noteDuration * 1.2);
    ledcWriteTone(BUZZERPIN, 0);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PyroelectricPIN, INPUT);

  lcd.init();
  lcd.backlight();

  // ESP32 buzzer channel
  ledcAttachChannel(BUZZERPIN, 2000, 8, 4);

  lcd.setCursor(0, 0);
  lcd.print("PIR Monitor");
  delay(1500);
  lcd.clear();
}

void loop() {

  int motion = digitalRead(PyroelectricPIN);

  // ---- detection latch logic (makes wider effective range) ----
  if (motion == HIGH) {
    lastDetectTime = millis();
    Serial.println("Motion raw trigger");
  }

  bool detected = (millis() - lastDetectTime) < HOLD_TIME;

  // ---- LCD update (no flicker) ----
  lcd.setCursor(0, 0);
  if (detected) {
    lcd.print("Motion Detected ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  } else {
    lcd.print("No Motion       ");
    lcd.setCursor(0, 1);
    lcd.print("Detected        ");
  }

  // ---- buzzer ----
  static bool alreadyPlayed = false;

  if (detected && !alreadyPlayed) {
    playAlert();
    alreadyPlayed = true;
  }

  if (!detected) {
    alreadyPlayed = false;
    ledcWriteTone(BUZZERPIN, 0);
  }

  delay(80);   // fast polling = more sensitive feel
}
