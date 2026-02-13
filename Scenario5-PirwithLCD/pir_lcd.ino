#include <Wire.h>
#include <U8g2lib.h>

#define PyroelectricPIN 23
#define LED 27
#define BuzzerPin 16
#define ButtonPin 5

// System flags
bool systemOn = true;
bool lastButtonState = HIGH;
bool buzzerState = false;

// OLED setup
U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ---- Fake setTextSize for U8g2 ----
void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) u8g2.setFont(u8g2_font_6x10_tr);
  else if (sizeLevel <= 1.5) u8g2.setFont(u8g2_font_ncenB14_tr);
  else if (sizeLevel <= 2.5) u8g2.setFont(u8g2_font_logisoso20_tr);
  else u8g2.setFont(u8g2_font_logisoso28_tr);
}

// ---- Buzzer melody ----
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_A4 440

int melody[] = {NOTE_E4, NOTE_G4, NOTE_A4};
int noteDurations[] = {4, 4, 4};

void playBuzzer() {
  for (int i = 0; i < sizeof(melody)/sizeof(int); i++) {
    int noteDuration = 1000 / noteDurations[i];
    tone(BuzzerPin, melody[i], noteDuration);
    delay(noteDuration * 1.3);
    noTone(BuzzerPin);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PyroelectricPIN, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(ButtonPin, INPUT_PULLUP);

  noTone(BuzzerPin);

  u8g2.begin();
}

void loop() {
  // ---- BUTTON TO TOGGLE SYSTEM ----
  bool buttonState = digitalRead(ButtonPin);
  if (lastButtonState == HIGH && buttonState == LOW) {
    systemOn = !systemOn;
    if (!systemOn) {
      digitalWrite(LED, LOW);
      noTone(BuzzerPin);
      buzzerState = false;
    }
    delay(200); // debounce
  }
  lastButtonState = buttonState;

  int ReadValue = digitalRead(PyroelectricPIN);

  if (systemOn && ReadValue) {
    // Motion detected
    Serial.println("Someone");
    digitalWrite(LED, HIGH);
    buzzerState = true;
    playBuzzer();
  } else {
    // No motion or system OFF
    Serial.println("No one");
    digitalWrite(LED, LOW);
    buzzerState = false;
    noTone(BuzzerPin);
  }

  // ---- OLED DISPLAY ----
  char line1[32], line2[32], line3[32];
  sprintf(line1, "System: %s", systemOn ? "ON" : "OFF");
  sprintf(line2, "Motion: %s", (ReadValue && systemOn) ? "Detected" : "No Motion");
  sprintf(line3, "Buzzer: %s", buzzerState ? "ON" : "OFF");

  u8g2.clearBuffer();
  setTextSize(1);
  u8g2.drawStr(33, 15, line1);
  u8g2.drawStr(33, 35, line2);
  u8g2.drawStr(33, 55, line3);
  u8g2.sendBuffer();

  delay(200);
}
