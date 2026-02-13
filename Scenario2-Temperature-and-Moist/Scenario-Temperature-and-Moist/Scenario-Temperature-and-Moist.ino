#include <Wire.h>
#include <U8g2lib.h>
#include <dht11.h>

#define DHT11PIN 17
#define LED 27
#define BUTTON 5

#define MotorPin1 19
#define MotorPin2 18

dht11 DHT11;

U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);


void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) u8g2.setFont(u8g2_font_6x10_tr);
  else if (sizeLevel <= 1.5) u8g2.setFont(u8g2_font_ncenB14_tr);
  else if (sizeLevel <= 2.5) u8g2.setFont(u8g2_font_logisoso20_tr);
  else u8g2.setFont(u8g2_font_logisoso28_tr);
}

char tempStr[25];
char humStr[25];
char fanStr[20];
char modeStr[20];

bool forceMode = false;
bool fanState = false;

bool lastButtonState = HIGH;
unsigned long pressStart = 0;
bool buttonHeld = false;

void setup() {
  u8g2.begin();

  pinMode(LED, OUTPUT);
  pinMode(MotorPin1, OUTPUT);
  pinMode(MotorPin2, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(MotorPin1, LOW);
  digitalWrite(MotorPin2, LOW);
}

void loop() {

  bool buttonState = digitalRead(BUTTON);

  // ---- BUTTON PRESS START ----
  if (lastButtonState == HIGH && buttonState == LOW) {
    pressStart = millis();
    buttonHeld = false;
  }

  // ---- BUTTON HOLD DETECT ----
  if (buttonState == LOW && !buttonHeld) {
    if (millis() - pressStart >= 800) {   // hold threshold
      buttonHeld = true;

      if (forceMode) {
        fanState = !fanState;  // manual toggle in FORCE mode
      }
    }
  }

  // ---- BUTTON RELEASE ----
  if (lastButtonState == LOW && buttonState == HIGH) {

    if (!buttonHeld) {
      forceMode = !forceMode; 
    }
  }

  lastButtonState = buttonState;


  int Temperature;
  int Humidity;

  DHT11.read(DHT11PIN);
  Temperature = DHT11.temperature;
  Humidity = DHT11.humidity;


  if (!forceMode) {
    if (Temperature > 10) fanState = true;
    else fanState = false;
  }


  if (fanState) {
    digitalWrite(LED, HIGH);
    digitalWrite(MotorPin1, HIGH);
    digitalWrite(MotorPin2, LOW);
    sprintf(fanStr, "FAN: ON");
  } else {
    digitalWrite(LED, LOW);
    digitalWrite(MotorPin1, LOW);
    digitalWrite(MotorPin2, LOW);
    sprintf(fanStr, "FAN: OFF");
  }


  if (forceMode) sprintf(modeStr, "MODE: FORCE");
  else sprintf(modeStr, "MODE: AUTO");


  sprintf(tempStr, "Temp: %d %cC", Temperature, 176);
  sprintf(humStr, "Hum: %d %%", Humidity);

  u8g2.clearBuffer();

  setTextSize(1);
  u8g2.drawStr(33, 15, tempStr);
  u8g2.drawStr(33, 35, humStr);
  u8g2.drawStr(33, 55, fanStr);
  u8g2.drawStr(33, 75, modeStr);

  u8g2.sendBuffer();

  delay(150);
}
