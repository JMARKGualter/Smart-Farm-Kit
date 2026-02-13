#include <Wire.h>
#include <U8g2lib.h>

#define RelayPin 25
#define WaterLevelPin 33
#define SoilHumidityPin 32
#define BUTTON 5

// Thresholds
int soilThreshold = 60;    
int waterThreshold = 10;   

// System flags
bool systemOn = true;       
bool lastButtonState = HIGH;
bool pumpState = false;

U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

char line1[32];
char line2[32];
char line3[32];

void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) {
    u8g2.setFont(u8g2_font_6x10_tr);
  } 
  else if (sizeLevel <= 1.5) {
    u8g2.setFont(u8g2_font_ncenB14_tr);
  } 
  else if (sizeLevel <= 2.5) {
    u8g2.setFont(u8g2_font_logisoso20_tr);
  } 
  else {
    u8g2.setFont(u8g2_font_logisoso28_tr);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(RelayPin, OUTPUT);
  pinMode(WaterLevelPin, INPUT);
  pinMode(SoilHumidityPin, INPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(RelayPin, LOW); 

  // Initialize OLED
  u8g2.begin();
}

void loop() {
  
  bool buttonState = digitalRead(BUTTON);
  if (lastButtonState == HIGH && buttonState == LOW) {
    systemOn = !systemOn;   
    if(!systemOn) {
      digitalWrite(RelayPin, LOW); 
      pumpState = false;
    }
    delay(200); 
  }
  lastButtonState = buttonState;


  int soilValue = analogRead(SoilHumidityPin);
  int waterValue = analogRead(WaterLevelPin);

  
  if (systemOn) {
    if (soilValue < soilThreshold && waterValue > waterThreshold) {
      digitalWrite(RelayPin, HIGH);
      pumpState = true;
    } else {
      digitalWrite(RelayPin, LOW);
      pumpState = false;
    }
  } else {
    digitalWrite(RelayPin, LOW);
    pumpState = false;
  }

  
  sprintf(line1, "System: %s", systemOn ? "ON" : "OFF");
  sprintf(line2, "Soil: %d %s", soilValue, (soilValue < soilThreshold) ? "LOW" : "OK");
  sprintf(line3, "Pump: %s  WL:%d", pumpState ? "ON" : "OFF", waterValue);


  u8g2.clearBuffer();
  setTextSize(1);
  u8g2.drawStr(33, 15, line1);
  u8g2.drawStr(33, 25, line2);
  u8g2.drawStr(33, 45, line3);
  u8g2.sendBuffer();

  delay(200);
}
