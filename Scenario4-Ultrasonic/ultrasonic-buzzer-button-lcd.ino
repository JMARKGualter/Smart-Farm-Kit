#include <Wire.h>
#include <U8g2lib.h>

#define TrigPin 12
#define EchoPin 13
#define ButtonPin 5
#define BuzzerPin 16

const int DIST_THRESHOLD = 5;

bool systemOn = true;
bool lastButtonState = HIGH;
bool buzzerState = false;
int melodyIndex = 0;      
unsigned long noteStart = 0;
bool notePlaying = false;


long duration;
int distance;


#define NOTE_E3 165
#define NOTE_G3 196
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440

int melody[] = {
  NOTE_E4, NOTE_E4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_G4, NOTE_G3,
  NOTE_C4, NOTE_G3, NOTE_E3, NOTE_A3, NOTE_B3, NOTE_AS3, NOTE_A3, NOTE_G3,
  NOTE_E4, NOTE_G4, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_E4, NOTE_C4, NOTE_D4, NOTE_B3
};

int noteDurations[] = {
  8,4,4,8,4,2,2,
  3,3,3,4,4,8,4,8,
  8,8,8,4,8,4,3,8,8
};


U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) u8g2.setFont(u8g2_font_6x10_tr);
  else if (sizeLevel <= 1.5) u8g2.setFont(u8g2_font_ncenB14_tr);
  else if (sizeLevel <= 2.5) u8g2.setFont(u8g2_font_logisoso20_tr);
  else u8g2.setFont(u8g2_font_logisoso28_tr);
}

void setup() {
  Serial.begin(9600);
  pinMode(TrigPin, OUTPUT);
  pinMode(EchoPin, INPUT);
  pinMode(ButtonPin, INPUT_PULLUP);
  pinMode(BuzzerPin, OUTPUT);
  noTone(BuzzerPin);
  u8g2.begin();
  noteStart = millis();
}

void loop() {
  
  bool buttonState = digitalRead(ButtonPin);
  if (lastButtonState == HIGH && buttonState == LOW) {
    systemOn = !systemOn;         
    buzzerState = false;
    melodyIndex = 0;
    notePlaying = false;
    noTone(BuzzerPin);
    delay(200);
  }
  lastButtonState = buttonState;

  
  if (systemOn) {
    digitalWrite(TrigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(TrigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(TrigPin, LOW);
    duration = pulseIn(EchoPin, HIGH);
    distance = duration / 58;

    if (distance <= DIST_THRESHOLD) {
      buzzerState = true;

     
      unsigned long now = millis();
      if (!notePlaying || (now - noteStart >= (1000 / noteDurations[melodyIndex]) * 1.3)) {
        tone(BuzzerPin, melody[melodyIndex]);
        noteStart = now;
        notePlaying = true;
        melodyIndex++;
        if (melodyIndex >= sizeof(melody)/sizeof(int)) melodyIndex = 0; // loop melody
      }

    } else {
      buzzerState = false;
      notePlaying = false;
      melodyIndex = 0;
      noTone(BuzzerPin);
    }

  } else {
    distance = 0;
    buzzerState = false;
    notePlaying = false;
    melodyIndex = 0;
    noTone(BuzzerPin);
  }

  char line1[32], line2[32], line3[32];
  sprintf(line1, "System: %s", systemOn ? "ON" : "OFF");
  sprintf(line2, "Distance: %d cm", distance);
  sprintf(line3, "Buzzer: %s", buzzerState ? "ON" : "OFF");

  u8g2.clearBuffer();
  setTextSize(1);
  u8g2.drawStr(33, 15, line1);
  u8g2.drawStr(33, 35, line2);
  u8g2.drawStr(33, 55, line3);
  u8g2.sendBuffer();

  delay(50);
}
