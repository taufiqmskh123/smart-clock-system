#include <WiFi.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= WIFI SETTINGS =================
// IMPORTANT: Ensure your Redmi-13C hotspot is set to "2.4 GHz Band"
const char* ssid     = "Redmi-13C";
const char* password = "taufiq@123";

// ================= TIME & ALARM SETTINGS =================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800; // India is UTC +5:30
const int   daylightOffset_sec = 0;

// SET YOUR ALARM TIME HERE (24-hour format)
const int targetHour = 23; 
const int targetMin  = 11; 
bool buzzerTriggered = false;

// ================= HARDWARE PINS =================
const int buzzerPin = 25; 
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // 1. Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED failed"));
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("System Booting...");
  display.display();
  delay(1000);

  // 2. Connect to WiFi with Timeout
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting WiFi...");
  display.display();

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if(WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.println("WiFi FAILED!");
    display.println("Check 2.4GHz Band");
    display.display();
    while(1); 
  }

  // 3. Sync Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  display.clearDisplay();
  display.println("WiFi Connected!");
  display.println("Syncing IST...");
  display.display();
}

void loop() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Waiting for NTP...");
    return;
  }

  // 4. Update OLED Display
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("IST TIME (INDIA)");

  // Main Time
  display.setTextSize(2);
  display.setCursor(15, 25);
  display.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  // Footer
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.printf("Alarm: %02d:%02d", targetHour, targetMin);
  
  display.display();

  // 5. Buzzer Logic (Triggers at targetHour:targetMin)
  if (timeinfo.tm_hour == targetHour && timeinfo.tm_min == targetMin && !buzzerTriggered) {
    
    // Visual Alert
    display.clearDisplay();
    display.setCursor(10, 20);
    display.setTextSize(2);
    display.println("EXAM START!");
    display.display();
    
    // Auditory Alert (10 Seconds)
    digitalWrite(buzzerPin, HIGH); 
    delay(10000); 
    digitalWrite(buzzerPin, LOW);
    
    buzzerTriggered = true; 
  }

  // Reset trigger flag once the minute has passed
  if (timeinfo.tm_min != targetMin) {
    buzzerTriggered = false;
  }

  delay(1000); // Main loop delay
}