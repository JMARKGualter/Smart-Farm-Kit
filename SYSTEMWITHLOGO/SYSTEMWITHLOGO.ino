#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include <dht11.h>

// ========== UNIVERSAL PIN DEFINITIONS ==========
#define BUTTON_PIN 5          
#define BUTTON_POWER_PIN 4   
#define OLED_SDA 21
#define OLED_SCL 22

// ========== SCENARIO 1: SOIL HUMIDITY & WATER LEVEL ==========
#define RELAY_PIN 25
#define WATER_LEVEL_PIN 33
#define SOIL_HUMIDITY_PIN 32
int soilThreshold = 60;
int waterThreshold = 10;

// ========== SCENARIO 2: HUMIDITY & FAN ==========
#define DHT11_PIN 17
#define LED_PIN 27
#define MOTOR_PIN1 19
#define MOTOR_PIN2 18
dht11 DHT11;

// ========== SCENARIO 3: SERVO DOOR ==========
#define SERVO_PIN 26
Servo myservo;
int STOP_POS = 90;
int OPEN_POS = 170;
int CLOSE_POS = 0;

// ========== SCENARIO 4: ULTRASONIC ==========
#define TRIG_PIN 12
#define ECHO_PIN 13
#define BUZZER_PIN 16
const int DIST_THRESHOLD = 5;

// ========== SCENARIO 5: PIR MOTION ==========
#define PYRO_PIN 23

// ========== SCENARIO 6: STEAM SENSOR ==========
#define STEAM_PIN 35
const int STEAM_THRESHOLD = 500;

// ========== SCENARIO 7: LIGHT SENSOR ==========
#define PHOTOCELL_PIN 34
const int LIGHT_THRESHOLD = 300;

// ========== SCENARIO MANAGEMENT ==========
#define NUM_SCENARIOS 7
#define SCENARIO_DURATION 15000  
unsigned long scenarioStartTime = 0;
int currentScenario = 0;
int previousScenario = -1;

// ========== SYSTEM STATE ==========
bool systemOn = false;        
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;
bool buttonHeld = false;
bool longPressProcessed = false;
const unsigned long HOLD_THRESHOLD = 800; 
const unsigned long DEBOUNCE_DELAY = 50;

// Array of all output pins that should be powered down when system is OFF
int outputPins[] = {
  RELAY_PIN,           // Scenario 1 pump
  MOTOR_PIN1,          // Scenario 2 fan
  MOTOR_PIN2,          // Scenario 2 fan
  LED_PIN,            // Scenario 2/5/6/7 LED
  SERVO_PIN,          // Scenario 3 servo
  BUZZER_PIN,         // Scenario 4/5/6 buzzer
  TRIG_PIN,           // Scenario 4 ultrasonic trigger
  // Note: ECHO_PIN is input, doesn't need power control
};
const int numOutputPins = sizeof(outputPins) / sizeof(outputPins[0]);

U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
const int TEXT_X = 33;  

// Scenario 1
bool pumpState = false;

// Scenario 2
bool forceMode = false;
bool fanState = false;

// Scenario 3
bool isOpen = false;
bool isMoving = false;
unsigned long servoMoveStartTime = 0;
int servoPosition = 90;
int targetPosition = 90;
const int servoMoveDuration = 2000; // 2 seconds to move

// Scenario 4
int ultrasonicDistance = 0;
bool buzzerState = false;
int melodyIndex = 0;
unsigned long noteStart = 0;
bool notePlaying = false;

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

// Scenario 5 & 6
bool alarmPlayed = false;

// Scenario 7
int lightValue = 0;
bool lightLEDState = false;

// ========== LOGO ANIMATION VARIABLES ==========
int centerX = 74;  // Moved +10 in X
int centerY = 64;
int size = 80;
int screenW = 128;
int screenH = 128;
unsigned long lastAnimationStep = 0;
int animationPhase = 0;
int animationStep = 0;
const int ANIMATION_DELAY = 20;
const int ANIMATION_STEPS = 20;
bool animationComplete = false;
unsigned long staticDisplayStart = 0;
const int STATIC_DISPLAY_DURATION = 5000; // 5 seconds static display

// ========== POWER MANAGEMENT FUNCTIONS ==========
void powerDownSystem() {
  Serial.println("POWER DOWN: All outputs disabled");
  
  for (int i = 0; i < numOutputPins; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }
  
  myservo.detach(); 
  
  // Reset all states
  pumpState = false;
  fanState = false;
  forceMode = false;
  isOpen = false;
  isMoving = false;
  servoPosition = STOP_POS;
  targetPosition = STOP_POS;
  buzzerState = false;
  notePlaying = false;
  alarmPlayed = false;
  lightLEDState = false;
  melodyIndex = 0;
}

void powerUpSystem() {
  Serial.println("POWER UP: All outputs enabled");
  
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(SOIL_HUMIDITY_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  
  myservo.attach(SERVO_PIN);
  myservo.write(STOP_POS);
  servoPosition = STOP_POS;
  targetPosition = STOP_POS;
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  
  pinMode(PYRO_PIN, INPUT);
  pinMode(STEAM_PIN, INPUT);
  pinMode(PHOTOCELL_PIN, INPUT);
  
  // Reset animation state
  animationPhase = 0;
  animationStep = 0;
  animationComplete = false;
}

// ========== OLED TEXT FUNCTIONS ==========
void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) u8g2.setFont(u8g2_font_6x10_tr);
  else if (sizeLevel <= 1.5) u8g2.setFont(u8g2_font_ncenB14_tr);
  else if (sizeLevel <= 2.5) u8g2.setFont(u8g2_font_logisoso20_tr);
  else u8g2.setFont(u8g2_font_logisoso28_tr);
}

void drawText(int y, const char* text) {
  u8g2.drawStr(TEXT_X, y, text);
}

// ========== FIXED LOGO ANIMATION FUNCTIONS ==========
void drawThickLine(int x1, int y1, int x2, int y2) {
  u8g2.drawLine(x1, y1, x2, y2);
  u8g2.drawLine(x1+1, y1, x2+1, y2);
}

void drawCompleteLogo() {
  int half = size / 2;
  
  int leftX   = centerX - half;
  int rightX  = centerX + half;
  int topY    = centerY - half;
  int bottomY = centerY + half;
  
  // Draw all 4 diamond segments
  drawThickLine(centerX, bottomY, leftX, centerY);  // Bottom to left
  drawThickLine(leftX, centerY, centerX, topY);      // Left to top
  drawThickLine(centerX, topY, rightX, centerY);     // Top to right
  drawThickLine(rightX, centerY, centerX, bottomY);  // Right to bottom
  
  // Draw the T (horizontal line through center)
  drawThickLine(leftX, centerY, rightX, centerY);
  
  // Draw the vertical line of T (from center to bottom)
  drawThickLine(centerX, centerY, centerX, bottomY);
}

void drawStaticLogoWithText() {
  u8g2.clearBuffer();
  drawCompleteLogo();
  
  // Add "TRIOE" text at (110, 100)
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(110, 100, "TRIOE");
  
  u8g2.sendBuffer();
}

void runLogoAnimation() {
  int half = size / 2;
  
  int leftX   = centerX - half;
  int rightX  = centerX + half;
  int topY    = centerY - half;
  int bottomY = centerY + half;
  
  // Segment endpoints for each animation phase
  int x1, y1, x2, y2;
  bool drawSegment1, drawSegment2, drawSegment3, drawSegment4, drawCross, drawVertical;
  
  // Set segment based on animation phase
  switch(animationPhase) {
    case 0: // First segment (bottom to left)
      x1 = centerX; y1 = bottomY;
      x2 = leftX; y2 = centerY;
      drawSegment1 = false; drawSegment2 = false; drawSegment3 = false; 
      drawSegment4 = false; drawCross = false; drawVertical = false;
      break;
    case 1: // Second segment (left to top)
      x1 = leftX; y1 = centerY;
      x2 = centerX; y2 = topY;
      drawSegment1 = true; drawSegment2 = false; drawSegment3 = false; 
      drawSegment4 = false; drawCross = false; drawVertical = false;
      break;
    case 2: // Third segment (top to right)
      x1 = centerX; y1 = topY;
      x2 = rightX; y2 = centerY;
      drawSegment1 = true; drawSegment2 = true; drawSegment3 = false; 
      drawSegment4 = false; drawCross = false; drawVertical = false;
      break;
    case 3: // Fourth segment (right to bottom)
      x1 = rightX; y1 = centerY;
      x2 = centerX; y2 = bottomY;
      drawSegment1 = true; drawSegment2 = true; drawSegment3 = true; 
      drawSegment4 = false; drawCross = false; drawVertical = false;
      break;
    case 4: // Cross segment (left to right)
      x1 = leftX; y1 = centerY;
      x2 = rightX; y2 = centerY;
      drawSegment1 = true; drawSegment2 = true; drawSegment3 = true; 
      drawSegment4 = true; drawCross = false; drawVertical = false;
      break;
    case 5: // Final segment - Vertical line (center to bottom) to complete the T
      x1 = centerX; y1 = centerY;
      x2 = centerX; y2 = bottomY;
      drawSegment1 = true; drawSegment2 = true; drawSegment3 = true; 
      drawSegment4 = true; drawCross = true; drawVertical = false;
      break;
    default:
      return;
  }
  
  // Handle animation timing
  if (animationPhase < 6) {
    // Still animating the segments
    if (animationStep < ANIMATION_STEPS) {
      // Drawing animation frame
      if (millis() - lastAnimationStep >= ANIMATION_DELAY) {
        animationStep++;
        lastAnimationStep = millis();
      }
      
      // Calculate interpolated position
      float t = (float)animationStep / ANIMATION_STEPS;
      int currentX = x1 + t * (x2 - x1);
      int currentY = y1 + t * (y2 - y1);
      
      // Draw frame
      u8g2.clearBuffer();
      
      // Draw completed segments
      if (drawSegment1) drawThickLine(centerX, bottomY, leftX, centerY);
      if (drawSegment2) drawThickLine(leftX, centerY, centerX, topY);
      if (drawSegment3) drawThickLine(centerX, topY, rightX, centerY);
      if (drawSegment4) drawThickLine(rightX, centerY, centerX, bottomY);
      if (drawCross) drawThickLine(leftX, centerY, rightX, centerY);
      
      // Draw current animating segment
      drawThickLine(x1, y1, currentX, currentY);
      
      u8g2.sendBuffer();
    } else {
      // Move to next phase
      animationPhase++;
      animationStep = 0;
    }
  } else {
    // Animation complete - show static logo with text for 5 seconds
    if (!animationComplete) {
      animationComplete = true;
      staticDisplayStart = millis();
    }
    
    // Draw static logo with TRIOE text
    drawStaticLogoWithText();
    
    // Check if 5 seconds have passed
    if (millis() - staticDisplayStart >= STATIC_DISPLAY_DURATION) {
      // Reset animation to start over
      animationPhase = 0;
      animationStep = 0;
      animationComplete = false;
    }
  }
}

// ========== FIXED BUTTON HANDLER ==========
void handleButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  // Button pressed (FALLING EDGE)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressTime = millis();
    buttonHeld = false;
    longPressProcessed = false;
    delay(DEBOUNCE_DELAY); // Simple debounce
  }
  
  // Button held - check for long press
  if (currentButtonState == LOW && !buttonHeld && !longPressProcessed) {
    if (millis() - buttonPressTime >= HOLD_THRESHOLD) {
      buttonHeld = true;
      longPressProcessed = true;
      
      Serial.println("LONG PRESS DETECTED");
      
      // ONLY process long press if system is ON
      if (systemOn) {
        switch(currentScenario) {
          case 0: // Scenario 1: Soil Humidity - No long press action
            break;
            
          case 1: // Scenario 2: Humidity & Fan - Toggle fan in FORCE mode
            if (forceMode) {
              fanState = !fanState;
              Serial.println("Fan toggled in FORCE mode");
            }
            break;
            
          case 2: // Scenario 3: Servo Door
            if (!isMoving) {
              if (!isOpen) {
                targetPosition = OPEN_POS;
                isOpen = true;
                Serial.println("Opening door");
              } else {
                targetPosition = CLOSE_POS;
                isOpen = false;
                Serial.println("Closing door");
              }
              isMoving = true;
              servoMoveStartTime = millis();
            }
            break;
            
          case 3: // Scenario 4: Ultrasonic - No long press action
          case 4: // Scenario 5: PIR Motion - No long press action
          case 5: // Scenario 6: Steam Sensor - No long press action
          case 6: // Scenario 7: Light Sensor - No long press action
            break;
        }
      }
    }
  }
  
  // Button released (RISING EDGE)
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    if (!buttonHeld && !longPressProcessed) {
      // SHORT PRESS: Toggle system ON/OFF
      Serial.println("SHORT PRESS DETECTED - Toggling system");
      systemOn = !systemOn;
      
      if (systemOn) {
        powerUpSystem();
        scenarioStartTime = millis();
        currentScenario = 0;
        previousScenario = -1;
      } else {
        powerDownSystem();
      }
    }
    delay(DEBOUNCE_DELAY); // Simple debounce
  }
  
  lastButtonState = currentButtonState;
}

// ========== SCENARIO TRANSITION HANDLER ==========
void handleScenarioTransition() {
  if (currentScenario != previousScenario) {
    Serial.print("Switching from scenario ");
    Serial.print(previousScenario);
    Serial.print(" to ");
    Serial.println(currentScenario);
    
    // Turning OFF previous scenario's actuators
    switch(previousScenario) {
      case 1:  // Scenario 2 - Force fan OFF when leaving
        digitalWrite(MOTOR_PIN1, LOW);
        digitalWrite(MOTOR_PIN2, LOW);
        digitalWrite(LED_PIN, LOW);
        fanState = false;
        forceMode = false;
        break;
        
      case 2:  // Scenario 3 - Stop servo at current position
        isMoving = false;
        break;
        
      case 3:  // Scenario 4 - Stop buzzer
      case 4:  // Scenario 5
      case 5:  // Scenario 6
      case 6:  // Scenario 7
        noTone(BUZZER_PIN);
        buzzerState = false;
        notePlaying = false;
        alarmPlayed = false;
        digitalWrite(LED_PIN, LOW);
        break;
    }
    
    // Reset common states
    melodyIndex = 0;
    notePlaying = false;
    alarmPlayed = false;
    noTone(BUZZER_PIN);
    
    previousScenario = currentScenario;
  }
}

// ========== SCENARIO FUNCTIONS ==========
void runScenario1() {
  int soilValue = analogRead(SOIL_HUMIDITY_PIN);
  int waterValue = analogRead(WATER_LEVEL_PIN);
  
  if (soilValue < soilThreshold && waterValue > waterThreshold) {
    digitalWrite(RELAY_PIN, HIGH);
    pumpState = true;
  } else {
    digitalWrite(RELAY_PIN, LOW);
    pumpState = false;
  }
  
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  if (timeLeft < 0) timeLeft = 0;
  
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 1:");
  drawText(30, "Soil Moisture");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  sprintf(line, "Soil: %d %s", soilValue, (soilValue < soilThreshold) ? "DRY" : "WET");
  drawText(65, line);
  sprintf(line, "Pump: %s", pumpState ? "ON" : "OFF");
  drawText(80, line);
  sprintf(line, "Water Lvl: %d", waterValue);
  drawText(95, line);
  drawText(120, "SHORT: OFF");
  u8g2.sendBuffer();
}

void runScenario2() {
  int Temperature = 0, Humidity = 0;
  int chk = DHT11.read(DHT11_PIN);
  
  if (chk == 0) {
    Temperature = DHT11.temperature;
    Humidity = DHT11.humidity;
  } else {
    Serial.print("DHT11 read error: ");
    Serial.println(chk);
  }
  
  if (!forceMode) {
    if (Temperature > 10) fanState = true;
    else fanState = false;
  }
  
  if (fanState) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(MOTOR_PIN1, HIGH);
    digitalWrite(MOTOR_PIN2, LOW);
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, LOW);
  }
  
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  if (timeLeft < 0) timeLeft = 0;
  
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 2:");
  drawText(30, "Temp & Humidity");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  
  if (chk == 0) {
    sprintf(line, "Temp: %d%cC", Temperature, 176);
    drawText(65, line);
    sprintf(line, "Hum: %d%%", Humidity);
    drawText(80, line);
  } else {
    drawText(65, "Temp: --°C");
    drawText(80, "Hum: --%");
    drawText(95, "Sensor Error");
  }
  
  sprintf(line, "Fan: %s", fanState ? "ON" : "OFF");
  drawText(110, line);
  sprintf(line, "Mode: %s", forceMode ? "FORCE" : "AUTO");
  drawText(125, line);
  
  u8g2.sendBuffer();
}

void runScenario3() {
  if (isMoving) {
    unsigned long elapsed = millis() - servoMoveStartTime;
    float progress = (float)elapsed / servoMoveDuration;
    
    if (progress >= 1.0) {
      myservo.write(targetPosition);
      servoPosition = targetPosition;
      isMoving = false;
    } else {
      int startPos = (targetPosition == OPEN_POS) ? CLOSE_POS : OPEN_POS;
      int currentPos = startPos + progress * (targetPosition - startPos);
      myservo.write(currentPos);
      servoPosition = currentPos;
    }
  } else {
    myservo.write(servoPosition);
  }
  
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  if (timeLeft < 0) timeLeft = 0;
  
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 3:");
  drawText(30, "Servo Door");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  sprintf(line, "Door: %s", isOpen ? "OPEN" : "CLOSED");
  drawText(65, line);
  sprintf(line, "Servo: %d", servoPosition);
  drawText(80, line);
  if (isMoving) drawText(95, "MOVING...");
  drawText(120, "LONG: Toggle Door");
  u8g2.sendBuffer();
}

void runScenario4() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration > 0) ultrasonicDistance = duration / 58;
  else ultrasonicDistance = 999;
  
  if (ultrasonicDistance <= DIST_THRESHOLD && ultrasonicDistance > 0) {
    buzzerState = true;
    unsigned long now = millis();
    if (!notePlaying || (now - noteStart >= (1000 / noteDurations[melodyIndex]) * 1.3)) {
      tone(BUZZER_PIN, melody[melodyIndex]);
      noteStart = now;
      notePlaying = true;
      melodyIndex++;
      if (melodyIndex >= sizeof(melody)/sizeof(int)) melodyIndex = 0;
    }
  } else {
    buzzerState = false;
    notePlaying = false;
    melodyIndex = 0;
    noTone(BUZZER_PIN);
  }
  
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  if (timeLeft < 0) timeLeft = 0;
  
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 4:");
  drawText(30, "Ultrasonic");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  sprintf(line, "Distance: %d cm", ultrasonicDistance);
  drawText(65, line);
  sprintf(line, "Buzzer: %s", buzzerState ? "ON" : "OFF");
  drawText(80, line);
  drawText(120, "SHORT: OFF");
  u8g2.sendBuffer();
}

void runScenario5() {
  int motionDetected = digitalRead(PYRO_PIN);
  
  if (motionDetected) {
    digitalWrite(LED_PIN, HIGH);
    buzzerState = true;
    if (!alarmPlayed) {
      tone(BUZZER_PIN, NOTE_E4, 200);
      alarmPlayed = true;
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    buzzerState = false;
    alarmPlayed = false;
    noTone(BUZZER_PIN);
  }
  
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  if (timeLeft < 0) timeLeft = 0;
  
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 5:");
  drawText(30, "PIR Motion");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  sprintf(line, "Motion: %s", motionDetected ? "DETECTED" : "None");
  drawText(65, line);
  sprintf(line, "Buzzer: %s", buzzerState ? "ON" : "OFF");
  drawText(80, line);
  drawText(120, "SHORT: OFF");
  u8g2.sendBuffer();
}

void runScenario6() {
  int steamValue = analogRead(STEAM_PIN);
  
  if (steamValue >= STEAM_THRESHOLD) {
    digitalWrite(LED_PIN, HIGH);
    if (!alarmPlayed) {
      buzzerState = true;
      tone(BUZZER_PIN, NOTE_G4, 300);
      alarmPlayed = true;
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    buzzerState = false;
    alarmPlayed = false;
    noTone(BUZZER_PIN);
  }
  
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  if (timeLeft < 0) timeLeft = 0;
  
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 6:");
  drawText(30, "Steam Sensor");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  sprintf(line, "Steam: %d", steamValue);
  drawText(65, line);
  sprintf(line, "Buzzer: %s", buzzerState ? "ON" : "OFF");
  drawText(80, line);
  drawText(120, "SHORT: OFF");
  u8g2.sendBuffer();
}

void runScenario7() {
  lightValue = analogRead(PHOTOCELL_PIN);
  
  if (lightValue < LIGHT_THRESHOLD) {
    digitalWrite(LED_PIN, HIGH);
    lightLEDState = true;
  } else {
    digitalWrite(LED_PIN, LOW);
    lightLEDState = false;
  }
  
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  if (timeLeft < 0) timeLeft = 0;
  
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 7:");
  drawText(30, "Light Sensor");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  sprintf(line, "Light: %d", lightValue);
  drawText(65, line);
  sprintf(line, "LED: %s", lightLEDState ? "ON" : "OFF");
  drawText(80, line);
  sprintf(line, "Env: %s", lightValue < LIGHT_THRESHOLD ? "DARK" : "BRIGHT");
  drawText(95, line);
  drawText(120, "SHORT: OFF");
  u8g2.sendBuffer();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  Serial.println("System Starting...");
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  u8g2.begin();
  
  for (int i = 0; i < numOutputPins; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }
  
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(SOIL_HUMIDITY_PIN, INPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PYRO_PIN, INPUT);
  pinMode(STEAM_PIN, INPUT);
  pinMode(PHOTOCELL_PIN, INPUT);
  
  systemOn = false;
  powerDownSystem();
  
  lastAnimationStep = millis();
  animationPhase = 0;
  animationStep = 0;
  
  Serial.println("Ready - Press button to start");
}

// ========== MAIN LOOP ==========
void loop() {
  handleButton();
  
  if (systemOn) {
    if (millis() - scenarioStartTime >= SCENARIO_DURATION) {
      currentScenario = (currentScenario + 1) % NUM_SCENARIOS;
      scenarioStartTime = millis();
      Serial.print("Switching to scenario ");
      Serial.println(currentScenario);
    }
    
    handleScenarioTransition();
    
    switch(currentScenario) {
      case 0: runScenario1(); break;
      case 1: runScenario2(); break;
      case 2: runScenario3(); break;
      case 3: runScenario4(); break;
      case 4: runScenario5(); break;
      case 5: runScenario6(); break;
      case 6: runScenario7(); break;
    }
  } else {
    runLogoAnimation();
  }
  
  delay(10);
}