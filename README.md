# 🏫 SynchroClock — Multi-Room Exam Timer System
 
A real-time exam management system using **ESP32 + ESP-NOW + Firebase + OLED displays**. A master ESP32 polls Firebase and wirelessly sends exam schedules to slave ESP32s in each classroom. Each slave shows the IST clock, counts down to the exam, runs the exam timer, and buzzes when time is up.
 
---
 
## 📸 System Overview
 
```
[Firebase] ←→ [Website Dashboard]
     ↓
[Master ESP32]  (WiFi + ESP-NOW)
     ↓ ESP-NOW
[Slave ESP32 - Room 101]    [Slave ESP32 - Room 102]
[OLED + Buzzer]             [OLED + Buzzer]
```
 
---
 
## 🧰 Hardware Required
 
| Component | Quantity |
|---|---|
| ESP32 Dev Board | 3 (1 master + 2 slaves) |
| SSD1306 OLED 128x64 (I2C) | 2 |
| Buzzer (active) | 2 |
| Jumper wires | as needed |
 
### Wiring (each slave)
| ESP32 Pin | Component |
|---|---|
| GPIO 25 | Buzzer + |
| GND | Buzzer − |
| GPIO 21 (SDA) | OLED SDA |
| GPIO 22 (SCL) | OLED SCL |
| 3.3V | OLED VCC |
| GND | OLED GND |
 
---
 
## 📁 File Structure
 
```
synchro-clock/
├── master/
│   └── master.cpp
├── slave_room101/
│   └── slave_room101.cpp
├── slave_room102/
│   └── slave_room102.cpp        ← same as 101, myRoom = "102"
├── website/
│   └── index.html
└── README.md
```
 
---
 
## ⚙️ Dependencies (PlatformIO / Arduino)
 
```ini
lib_deps =
  adafruit/Adafruit SSD1306
  adafruit/Adafruit GFX Library
  bblanchon/ArduinoJson
  esp32 arduino core (includes ESP32_NOW, Preferences, WiFi)
```
 
---
 
## 🔥 Firebase Setup
 
1. Create a Firebase project at [console.firebase.google.com](https://console.firebase.google.com)
2. Enable **Realtime Database** (asia-southeast1 region recommended)
3. Set database rules to allow read/write (for testing):
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```
4. Create this structure manually or via the website:
```
rooms/
  101/
    active: false
    hour: 0
    minute: 0
    subject: "Math"
    duration: 120
  102/
    active: false
    hour: 0
    minute: 0
    subject: "Physics"
    duration: 90
```
 
---
 
## 🚀 Setup Steps
 
### 1. Find each slave's MAC address
Upload this to each slave ESP32:
```cpp
#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  delay(1000);
  WiFi.begin();
  delay(1000);
  Serial.println(WiFi.macAddress());
}
void loop() {}
```
Note the MAC and update `master.cpp`:
```cpp
Classroom classrooms[] = {
  {"101", {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}, false},
  {"102", {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}, false}
};
```
 
### 2. Check WiFi channel
Upload this to master:
```cpp
#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  delay(1000);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.println("Channel: " + String(WiFi.channel()));
}
void loop() {}
```
Update `WiFi.setChannel(X)` in both slave files to match.
 
### 3. Clear NVS on master (first time only)
Upload this once before flashing real master code:
```cpp
#include <Preferences.h>
Preferences prefs;
void setup() {
  Serial.begin(115200);
  prefs.begin("rooms", false);
  prefs.clear();
  prefs.end();
  Serial.println("NVS cleared!");
}
void loop() {}
```
 
### 4. Update credentials in all files
In `master.cpp` and both slave files:
```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```
In `master.cpp`:
```cpp
const String FIREBASE_HOST = "https://YOUR_PROJECT.firebasedatabase.app/rooms.json";
const String FIREBASE_AUTH = "YOUR_DATABASE_SECRET";
```
 
### 5. Flash all three ESP32s
- Flash `master.cpp` → master board
- Flash `slave_room101.cpp` → Room 101 board
- Flash `slave_room102.cpp` → Room 102 board (only difference: `myRoom = "102"`)
---
 
## 🖥️ Website Dashboard
 
Open `index.html` in any browser. Update Firebase config inside the script tag with your project credentials. Features:
- Set subject, hour, minute, duration per room
- Abort exam button (sets `active: false` without wiping other fields)
---
 
## 📋 How to Trigger an Exam (every time)
 
> ⚠️ Always follow this exact order
 
1. Set `active: false` in Firebase (or click Abort) → wait 6 seconds
2. Set `hour`, `minute`, `subject`, `duration`
3. Set `active: true`
4. Serial Monitor on master should print: `Sent exam to Room 101`
**Timing tip:** Set exam time at least 30 seconds ahead of current IST. The OLED shows a 10-second countdown before the exam starts.
 
---
 
## 🔄 System Behaviour
 
| State | OLED Shows | Buzzer |
|---|---|---|
| Boot / Idle | Room name + IST clock | Silent |
| 10s before exam | Subject + countdown seconds | Silent |
| Exam starting | "EXAM STARTING!" | 10s ring |
| Exam running | MM:SS timer + subject + duration | Silent |
| Exam over | "TIME'S UP!" | 5s ring |
| After exam | Back to IST clock | Silent |
| Abort received | Back to IST clock immediately | Silent |
 
---
 
## 🛠️ Troubleshooting
 
| Problem | Fix |
|---|---|
| Slave shows "WAITING FOR SYNC" | Check WiFi credentials in slave, NTP sync failed |
| Master never prints "Sent exam" | Clear NVS, ensure active went false → true |
| Room 102 not receiving | Verify MAC address, verify channel matches master |
| Time is wrong | Ensure `+19800` offset applied (UTC → IST) |
| Exam re-triggers on EN reset | NVS stores sent state — survives reboot |
| Want to cancel mid-exam | Set `active: false` on website → slave returns to clock |
 
---
 
## 📡 ESP-NOW Packet Types
 
| Packet | Size | Purpose |
|---|---|---|
| `heartbeat_message` | 5 bytes | Clock sync every 3s |
| `struct_message` | ~33 bytes | Exam schedule |
| `cancel_message` | 1 byte | Abort exam |
 
---
 
## 👤 Author
 
Made by **Taufiq** — IoT exam management system for multi-room scheduling
