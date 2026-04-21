#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"

// --- CONFIGURATION ---
#define WIFI_SSID "Redmi-13C"
#define WIFI_PASSWORD "taufiq@123"
#define FIREBASE_HOST "synchro-clock-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "bO9liiAmBtArBjXV7AmK9vbWBoSipAgFiEQ7oCi8"
#define ROOM_ID "102" // Change this to "101" for the other board

const int buzzerPin = 25;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String examSubject = "WAITING...";
bool isAlert = false;

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // OLED Init
  delay(1000);
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Firebase Init
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);

  // Time Init (IST)
  configTime(19800, 0, "pool.ntp.org");
}

void loop() {
  struct tm info;
  if (!getLocalTime(&info)) {
    Serial.println("Failed to obtain time");
  }

  // Check Firebase for this specific room
  String path = "/rooms/" + String(ROOM_ID);
  if (Firebase.getBool(fbdo, path + "/active") && fbdo.boolData()) {
    int h = Firebase.getInt(fbdo, path + "/hour") ? fbdo.intData() : -1;
    int m = Firebase.getInt(fbdo, path + "/minute") ? fbdo.intData() : -1;

    if (info.tm_hour == h && info.tm_min == m) {
      Firebase.getString(fbdo, path + "/subject");
      examSubject = fbdo.stringData();
      isAlert = true;
      
      // Ring Buzzer
      digitalWrite(buzzerPin, HIGH);
      delay(3000);
      digitalWrite(buzzerPin, LOW);
      
      // Reset Alert in Firebase so it doesn't ring forever
      Firebase.setBool(fbdo, path + "/active", false);
    }
  }

  // Update Display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("STATION: "); display.print(ROOM_ID);
  display.drawLine(0, 10, 128, 10, WHITE);

  display.setTextSize(2);
  display.setCursor(15, 25);
  display.printf("%02d:%02d:%02d", info.tm_hour, info.tm_min, info.tm_sec);
  
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("EXAM: "); display.print(isAlert ? examSubject : "READY");
  display.display();

  delay(1000); // Check every second
}