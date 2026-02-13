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
const unsigned long HOLD_THRESHOLD = 800; 

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
unsigned long servoMoveStartTime = 0;
int servoPosition = 90;
const int servoMoveDuration = 2000;

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


void powerDownSystem() {
  Serial.println("POWER DOWN: All outputs disabled");
  
  for (int i = 0; i < numOutputPins; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }
  
  myservo.detach(); 
  
 
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  u8g2.setPowerSave(1); 
  
 
  pumpState = false;
  fanState = false;
  forceMode = false;
  isOpen = false;
  servoPosition = STOP_POS;
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
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  
  pinMode(PYRO_PIN, INPUT);
  pinMode(STEAM_PIN, INPUT);
  pinMode(PHOTOCELL_PIN, INPUT);
  
  
  u8g2.setPowerSave(0);
  
  
}


void setTextSize(float sizeLevel) {
  if (sizeLevel <= 1.0) u8g2.setFont(u8g2_font_6x10_tr);
  else if (sizeLevel <= 1.5) u8g2.setFont(u8g2_font_ncenB14_tr);
  else if (sizeLevel <= 2.5) u8g2.setFont(u8g2_font_logisoso20_tr);
  else u8g2.setFont(u8g2_font_logisoso28_tr);
}


void drawText(int y, const char* text) {
  u8g2.drawStr(TEXT_X, y, text);
}

void handleButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressTime = millis();
    buttonHeld = false;
  }
  
  
  if (currentButtonState == LOW && !buttonHeld) {
    if (millis() - buttonPressTime >= HOLD_THRESHOLD) {
      buttonHeld = true;
      
      
      if (systemOn) {
       
        switch(currentScenario) {
          case 0:  // Scenario 1: Soil Humidity - No long press action
            break;
            
          case 1:  // Scenario 2: Humidity & Fan - Toggle fan in FORCE mode
            if (forceMode) {
              fanState = !fanState;
            }
            break;
            
          case 2:  // Scenario 3: Servo Door - Toggle door
            if (!isOpen) {
              servoPosition = OPEN_POS;
              servoMoveStartTime = millis();
            } else {
              servoPosition = CLOSE_POS;
              servoMoveStartTime = millis();
            }
            break;
            
          case 3:  // Scenario 4: Ultrasonic - No long press action
          case 4:  // Scenario 5: PIR Motion - No long press action
          case 5:  // Scenario 6: Steam Sensor - No long press action
          case 6:  // Scenario 7: Light Sensor - No long press action
            break;
        }
      }
    }
  }
  
  // Button released
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    if (!buttonHeld) {
      // SHORT PRESS: Toggle system ON/OFF (universal)
      systemOn = !systemOn;
      
      if (systemOn) {
        // Power up all peripherals
        powerUpSystem();
        // Reset scenario timer when turning on
        scenarioStartTime = millis();
        currentScenario = 0;
        previousScenario = -1;
      } else {
        // Power down all peripherals
        powerDownSystem();
      }
    }
  }
  
  lastButtonState = currentButtonState;
}

// ========== SCENARIO TRANSITION HANDLER ==========
void handleScenarioTransition() {
  if (currentScenario != previousScenario) {
    // Turning OFF previous scenario's actuators
    switch(previousScenario) {
      case 1:  // Scenario 2 - Force fan OFF when leaving
        digitalWrite(MOTOR_PIN1, LOW);
        digitalWrite(MOTOR_PIN2, LOW);
        digitalWrite(LED_PIN, LOW);
        fanState = false;
        forceMode = false;
        break;
        
      case 2:  // Scenario 3 - Stop servo
        myservo.write(STOP_POS);
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
  // Read sensors
  int soilValue = analogRead(SOIL_HUMIDITY_PIN);
  int waterValue = analogRead(WATER_LEVEL_PIN);
  
  // Control pump
  if (soilValue < soilThreshold && waterValue > waterThreshold) {
    digitalWrite(RELAY_PIN, HIGH);
    pumpState = true;
  } else {
    digitalWrite(RELAY_PIN, LOW);
    pumpState = false;
  }
  
  // Calculate countdown
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  
  // Display - All text at X = 33
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
  // Read sensor
  int Temperature = 0, Humidity = 0;
  DHT11.read(DHT11_PIN);
  Temperature = DHT11.temperature;
  Humidity = DHT11.humidity;
  
  // Auto mode logic
  if (!forceMode) {
    if (Temperature > 10) fanState = true;
    else fanState = false;
  }
  
  // Apply fan state
  if (fanState) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(MOTOR_PIN1, HIGH);
    digitalWrite(MOTOR_PIN2, LOW);
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, LOW);
  }
  
  // Calculate countdown
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  
  // Display - All text at X = 33
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 2:");
  drawText(30, "Temp & Humidity");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  
  sprintf(line, "Temp: %d%cC", Temperature, 176);
  drawText(65, line);
  sprintf(line, "Hum: %d%%", Humidity);
  drawText(80, line);
  sprintf(line, "Fan: %s", fanState ? "ON" : "OFF");
  drawText(95, line);
  sprintf(line, "Mode: %s", forceMode ? "FORCE" : "AUTO");
  drawText(110, line);
  
  drawText(128, "LONG: Toggle Fan");
  u8g2.sendBuffer();
}

void runScenario3() {
  // Stop servo after duration
  if (millis() - servoMoveStartTime >= servoMoveDuration) {
    myservo.write(STOP_POS);
    isOpen = (servoPosition == OPEN_POS);
  }
  
  // Write servo position
  myservo.write(servoPosition);
  
  // Calculate countdown
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  
  // Display - All text at X = 33
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(15, "SCENARIO 3:");
  drawText(30, "Servo Door");
  
  char line[32];
  sprintf(line, "Next: %ds", timeLeft);
  drawText(45, line);
  
  sprintf(line, "Door: %s", isOpen ? "OPEN" : "CLOSED");
  drawText(65, line);
  sprintf(line, "Servo Pos: %d", servoPosition);
  drawText(80, line);
  
  drawText(120, "LONG: Toggle Door");
  u8g2.sendBuffer();
}

void runScenario4() {
  // Read ultrasonic
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration > 0) {
    ultrasonicDistance = duration / 58;
  } else {
    ultrasonicDistance = 999;
  }
  
  // Play melody if object detected
  if (ultrasonicDistance <= DIST_THRESHOLD) {
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
  
  // Calculate countdown
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  
  // Display - All text at X = 33
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
  
  // Calculate countdown
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  
  // Display - All text at X = 33
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
  
  // Calculate countdown
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  
  // Display - All text at X = 33
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
  // Read light sensor
  lightValue = analogRead(PHOTOCELL_PIN);
  
  // Control LED based on light level
  if (lightValue < LIGHT_THRESHOLD) {
    digitalWrite(LED_PIN, HIGH);
    lightLEDState = true;
  } else {
    digitalWrite(LED_PIN, LOW);
    lightLEDState = false;
  }
  
  // Calculate countdown
  int timeLeft = (SCENARIO_DURATION - (millis() - scenarioStartTime)) / 1000;
  
  // Display - All text at X = 33
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
  
  // Show environment status
  sprintf(line, "Env: %s", lightValue < LIGHT_THRESHOLD ? "DARK" : "BRIGHT");
  drawText(95, line);
  
  drawText(120, "SHORT: OFF");
  u8g2.sendBuffer();
}

// Display power-off screen
void showPowerOffScreen() {
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(40, "SYSTEM OFF");
  drawText(65, "Press button");
  drawText(80, "to power ON");
  
  // Simple animation - blinking dot
  static unsigned long lastBlink = 0;
  static bool blinkState = false;
  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink = millis();
  }
  
  if (blinkState) {
    drawText(100, "●");
  }
  
  u8g2.sendBuffer();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  
  // Initialize button pin (always active)
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize OLED
  u8g2.begin();
  
  // Initialize all output pins to LOW state
  for (int i = 0; i < numOutputPins; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }
  
  // Initialize input pins
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(SOIL_HUMIDITY_PIN, INPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PYRO_PIN, INPUT);
  pinMode(STEAM_PIN, INPUT);
  pinMode(PHOTOCELL_PIN, INPUT);
  
  // Start with system OFF (power saving mode)
  systemOn = false;
  powerDownSystem();
  
  // Welcome message - will show briefly then power down
  u8g2.setPowerSave(0);
  u8g2.clearBuffer();
  setTextSize(1);
  drawText(40, "7-in-1 Sensor");
  drawText(70, "System Ready");
  drawText(100, "Press to Start");
  u8g2.sendBuffer();
  delay(2000);
  
  // Power down until button press
  powerDownSystem();
}

// ========== MAIN LOOP ==========
void loop() {
  // Handle button presses (always active)
  handleButton();
  
  if (systemOn) {
    // Normal operation - all peripherals powered
    
    // Check if it's time to switch scenario
    if (millis() - scenarioStartTime >= SCENARIO_DURATION) {
      currentScenario = (currentScenario + 1) % NUM_SCENARIOS;
      scenarioStartTime = millis();
    }
    
    // Handle scenario transition
    handleScenarioTransition();
    
    // Run current scenario
    switch(currentScenario) {
      case 0:
        runScenario1();
        break;
      case 1:
        runScenario2();
        break;
      case 2:
        runScenario3();
        break;
      case 3:
        runScenario4();
        break;
      case 4:
        runScenario5();
        break;
      case 5:
        runScenario6();
        break;
      case 6:
        runScenario7();
        break;
    }
  } else {
    // System OFF - show power off screen
    showPowerOffScreen();
    delay(100);  
  }
  
  delay(20);  
}