/* ====================================================================
 * PROJECT:    IoT-Based Smart Inhaler (Team ELECTROX)
 * PLATFORM:   ESP32 DevKit V1
 * FEATURES:   FSR Pressure Sensing, SSD1306 OLED Mirror, 
 *             3-Puff/10s Emergency Detection, Web Serial JSON Stream
 * ==================================================================== */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------------------------------------------------------------
// PIN & HARDWARE DEFINITIONS
// --------------------------------------------------------------------
#define FSR_PIN           34    // Analog Input pin for Force Sensitive Resistor
#define BUZZER_PIN        25    // Digital Output pin for Active Buzzer
#define SDA_PIN           21    // I2C SDA Pin
#define SCL_PIN           22    // I2C SCL Pin

#define SCREEN_WIDTH      128   // OLED width in pixels
#define SCREEN_HEIGHT     64    // OLED height in pixels
#define OLED_RESET        -1    // Reset pin (-1 if sharing ESP32 reset)
#define OLED_ADDRESS      0x3C  // I2C Address for 0.96" SSD1306 OLED

// --------------------------------------------------------------------
// ALGORITHM THRESHOLDS & CONSTANTS
// --------------------------------------------------------------------
#define FSR_THRESHOLD     500   // ADC trigger value (Range: 0 - 4095)
#define TIME_WINDOW_MS    10000 // 10 Seconds window for emergency check (in ms)
#define MAX_PUFF_HISTORY  20    // Circular buffer size for timestamps

// --------------------------------------------------------------------
// GLOBAL OBJECTS & VARIABLES
// --------------------------------------------------------------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int totalPuffs = 0;
bool isPressed = false;
int currentFsrReading = 0;

// Sliding Time-Window Timestamp Storage
unsigned long puffTimestamps[MAX_PUFF_HISTORY];
int timestampCount = 0;

// Emergency Siren State
bool isEmergency = false;
unsigned long emergencyStartTime = 0;
const unsigned long EMERGENCY_DURATION = 4000; // Alarm duration in ms (4 seconds)

// Serial JSON Broadcast Rate Limiter
unsigned long lastSerialSendTime = 0;
const unsigned long SERIAL_INTERVAL = 100; // Broadcast JSON every 100ms (10Hz)

// --------------------------------------------------------------------
// FUNCTION DECLARATIONS
// --------------------------------------------------------------------
void recordPuff(unsigned long currentTime);
int getPuffsInWindow(unsigned long currentTime);
void triggerEmergency();
void handleBuzzerAlerts(unsigned long currentTime);
void updateOLEDDisplay();
void streamJSONData();

// --------------------------------------------------------------------
// SETUP FUNCTION
// --------------------------------------------------------------------
void setup() {
  // Initialize High-Speed Serial Output for Web Serial API
  Serial.begin(115200);

  // Pin Modes
  pinMode(FSR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize I2C Communication
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize SSD1306 OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("OLED Allocation Failed! Check Wiring."));
    for (;;); // Stop execution if OLED missing
  }

  // Display Splash Screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 10);
  display.println(F("TEAM ELECTROX"));
  display.setCursor(10, 30);
  display.setTextSize(2);
  display.println(F("SMART INHALER"));
  display.setCursor(20, 50);
  display.setTextSize(1);
  display.println(F("SYSTEM READY"));
  display.display();
  delay(2000);
}

// --------------------------------------------------------------------
// MAIN LOOP
// --------------------------------------------------------------------
void loop() {
  unsigned long currentTime = millis();
  
  // 1. Read Analog Pressure Sensor (FSR)
  currentFsrReading = analogRead(FSR_PIN);

  // 2. Debouncing & Edge-Detection Logic (State Machine)
  if (currentFsrReading > FSR_THRESHOLD && !isPressed) {
    // RISING EDGE DETECTED: New Puff Event!
    isPressed = true;
    totalPuffs++;
    
    // Play quick single confirmation beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(80);
    digitalWrite(BUZZER_PIN, LOW);

    // Record timestamp and check for Overdose Condition
    recordPuff(currentTime);

  } else if (currentFsrReading <= FSR_THRESHOLD && isPressed) {
    // FALLING EDGE DETECTED: Canister Released
    isPressed = false;
  }

  // 3. Handle Non-blocking Emergency Siren Buzzer Logic
  handleBuzzerAlerts(currentTime);

  // 4. Send JSON Payload over Serial to Web Dashboard
  if (currentTime - lastSerialSendTime >= SERIAL_INTERVAL) {
    streamJSONData();
    lastSerialSendTime = currentTime;
  }

  // 5. Update Local Physical OLED Screen
  updateOLEDDisplay();
  
  delay(10); // Small loop stability delay
}

// --------------------------------------------------------------------
// RECORD PUFF & OVERDOSE SLIDING-WINDOW ALGORITHM
// --------------------------------------------------------------------
void recordPuff(unsigned long currentTime) {
  // Store timestamp in ring buffer
  if (timestampCount < MAX_PUFF_HISTORY) {
    puffTimestamps[timestampCount] = currentTime;
    timestampCount++;
  } else {
    // Shift left to make room for newest timestamp
    for (int i = 0; i < MAX_PUFF_HISTORY - 1; i++) {
      puffTimestamps[i] = puffTimestamps[i + 1];
    }
    puffTimestamps[MAX_PUFF_HISTORY - 1] = currentTime;
  }

  // Calculate puffs within the 10-second window
  int recentPuffs = getPuffsInWindow(currentTime);

  // Check Overdose Threshold (3 or more puffs in 10 seconds)
  if (recentPuffs >= 3) {
    triggerEmergency();
  }
}

// Returns the count of puffs occurring within the last TIME_WINDOW_MS (10 seconds)
int getPuffsInWindow(unsigned long currentTime) {
  int count = 0;
  for (int i = 0; i < timestampCount; i++) {
    if ((currentTime - puffTimestamps[i]) <= TIME_WINDOW_MS) {
      count++;
    }
  }
  return count;
}

void triggerEmergency() {
  isEmergency = true;
  emergencyStartTime = millis();
}

// --------------------------------------------------------------------
// NON-BLOCKING BUZZER ALARM (SIREN EFFECT)
// --------------------------------------------------------------------
void handleBuzzerAlerts(unsigned long currentTime) {
  if (isEmergency) {
    if (currentTime - emergencyStartTime <= EMERGENCY_DURATION) {
      // Rapid pulse alarm for emergency condition
      bool state = ((currentTime / 100) % 2 == 0);
      digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
    } else {
      // Emergency alarm time expired
      isEmergency = false;
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

// --------------------------------------------------------------------
// OLED RENDERER
// --------------------------------------------------------------------
void updateOLEDDisplay() {
  display.clearDisplay();

  if (isEmergency) {
    // EMERGENCY SCREEN STATE
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(14, 12);
    display.setTextSize(2);
    display.println(F("OVERDOSE!"));
    display.setTextSize(1);
    display.setCursor(10, 38);
    display.println(F("> 3 Puffs in 10s <"));
    display.setCursor(20, 50);
    display.println(F("PAUSE INHALATION"));
  } else {
    // NORMAL MONITORING STATE
    display.setTextColor(SSD1306_WHITE);
    
    // Header Bar
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("SMART INHALER v1.0"));
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    // Total Puff Display
    display.setCursor(0, 20);
    display.println(F("Puffs Count:"));
    display.setCursor(80, 16);
    display.setTextSize(2);
    display.println(totalPuffs);

    // Pressure Reading Display
    display.setTextSize(1);
    display.setCursor(0, 42);
    display.println(F("FSR Press:"));
    display.setCursor(80, 42);
    display.println(currentFsrReading);

    // Dynamic Pressure Bar Graph at bottom
    int barWidth = map(constrain(currentFsrReading, 0, 4095), 0, 4095, 0, 128);
    display.fillRect(0, 56, barWidth, 8, SSD1306_WHITE);
  }

  display.display();
}

// --------------------------------------------------------------------
// SERIAL JSON BROADCASTER (For Web Serial Dashboard)
// --------------------------------------------------------------------
void streamJSONData() {
  // Output format: {"puffs":3,"fsr":820}
  Serial.print("{\"puffs\":");
  Serial.print(totalPuffs);
  Serial.print(",\"fsr\":");
  Serial.print(currentFsrReading);
  Serial.println("}");
}