#include <esp_now.h>
#include <WiFi.h>
#include "time.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int buzzerPin = 25;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// CHANGE THIS for each Slave (e.g., "Room 101" or "Room 102")
String roomLabel = "Room 101"; 
String activeSubject = "WAITING...";

typedef struct struct_message {
    char subject[32];
    bool trigger;
} struct_message;

struct_message incoming;

void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
  memcpy(&incoming, data, sizeof(incoming));
  if(incoming.trigger) {
    activeSubject = String(incoming.subject);
    digitalWrite(buzzerPin, HIGH);
    delay(5000); // Ring for 5 seconds
    digitalWrite(buzzerPin, LOW);
  }
}

void setup() {
  pinMode(buzzerPin, OUTPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  WiFi.mode(WIFI_STA);
  // Optional: Sync time once at start using Hotspot
  WiFi.begin("Redmi-13C", "taufiq@123");
  int timeout = 0;
  while(WiFi.status() != WL_CONNECTED && timeout < 10) { delay(500); timeout++; }
  configTime(19800, 0, "pool.ntp.org");

  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  }
}

void loop() {
  struct tm timeinfo;
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(roomLabel);

  if(getLocalTime(&timeinfo)) {
    display.setTextSize(2);
    display.setCursor(15, 20);
    display.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  }

  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print("EXAM: ");
  display.print(activeSubject);
  
  display.display();
  delay(1000);
}