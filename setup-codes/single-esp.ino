#include <WiFi.h>
#include <FirebaseESP32.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= CREDENTIALS =================
#define WIFI_SSID "Redmi-13C"
#define WIFI_PASSWORD "taufiq@123"
#define FIREBASE_HOST "synchro-clock-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET_HERE" // Get this from Project Settings -> Service Accounts

// ================= SETTINGS =================
const int buzzerPin = 25;
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

int targetHour = -1, targetMin = -1;
String subjectName = "Loading...";
bool alarmActive = false;
bool buzzerTriggered = false;

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  
  // OLED Init
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("OLED Failed");
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.println("Connecting WiFi...");
  display.display();

  // WiFi Connect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  // Firebase Config
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // NTP Sync (India)
  configTime(19800, 0, "pool.ntp.org");
}

void loop() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  // 1. DIRECT DATA FETCH FROM FIREBASE
  // We fetch values individually to avoid JSON parsing errors
  if (Firebase.getInt(fbdo, "/alarm/hour")) {
    targetHour = fbdo.intData();
  }
  if (Firebase.getInt(fbdo, "/alarm/minute")) {
    targetMin = fbdo.intData();
  }
  if (Firebase.getString(fbdo, "/alarm/subject")) {
    subjectName = fbdo.stringData();
  }
  if (Firebase.getBool(fbdo, "/alarm/active")) {
    alarmActive = fbdo.boolData();
  }

  display.clearDisplay();
  
  // 2. ALARM TRIGGER LOGIC
  if (alarmActive && timeinfo.tm_hour == targetHour && timeinfo.tm_min == targetMin && !buzzerTriggered) {
    
    // Display Alarm State
    display.setTextSize(2);
    display.setCursor(0, 5);
    display.println("ALARM!");
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.println(subjectName);
    display.display();

    // Sound Alarm (10 Seconds)
    digitalWrite(buzzerPin, HIGH);
    delay(10000); 
    digitalWrite(buzzerPin, LOW);
    
    // Tell Firebase to deactivate the alarm so it doesn't loop
    Firebase.setBool(fbdo, "/alarm/active", false);
    buzzerTriggered = true;
  } 
  else {
    // 3. NORMAL OPERATION DISPLAY
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("IST TIME (WCE)");
    
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("Next Exam: ");
    display.println(subjectName);
    
    // Reset trigger flag when minute changes
    if(timeinfo.tm_min != targetMin) buzzerTriggered = false;
  }
  
  display.display();
  delay(1000); // Poll every second for better responsiveness
}