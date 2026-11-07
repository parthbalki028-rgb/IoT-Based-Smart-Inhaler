# 🏥 IoT-Based Smart Inhaler | Team ELECTROX

> **Smarter Inhalation, Better Healthcare.**  
> An integrated IoT healthcare system combining ESP32 pressure telemetry, Web Serial API real-time dashboarding, and overdose emergency protocols.

---

## ⚡ Key Features
- **Real-Time Web Serial Telemetry:** High-speed JSON streaming (115200 Baud) directly to browser without Wi-Fi/Cloud lag.
- 🚨 **Emergency Overdose Protocol:** Sliding time-window algorithm detecting 3+ puffs in 10 seconds.
- 🤖 **JARVIS Voice Assistant:** Integrated Web Speech API vocal guidance.
- 📟 **Virtual OLED Mirror:** Live browser replication of the physical SSD1306 display.

---

## 🛠️ Hardware Components
1. **ESP32 DevKit V1** (Microcontroller)
2. **0.96" SSD1306 OLED Display** (Visual Feedback)
3. **FSR-402 Pressure Sensor** (Puff Trigger)
4. **3.7V Li-Ion Battery & TP4056** (Power Management)
5. **Active Buzzer** (Emergency Audio Alerts)

---

## 💻 Tech Stack
- **Embedded Software:** C++ / Arduino IDE
- **Frontend Web:** HTML5, Modern CSS3, JavaScript (ES6+), Web Serial API, Web Audio API
