#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ESP32_NOW.h"
#include <time.h>
#include <Preferences.h>

const char *ssid     = "-";
const char *password = "-";

const String FIREBASE_HOST = "-";
const String FIREBASE_AUTH = "-";

Preferences prefs;

struct Classroom {
  String roomName;
  uint8_t mac[6];
  bool lastActiveState;
};

Classroom classrooms[] = {
  {"101", {0x30, 0x76, 0xF5, 0xB9, 0xD8, 0xF8}, false},
  {"102", {0x8C, 0x94, 0xDF, 0x92, 0xFA, 0x78}, false}
};
int numSlaves = sizeof(classrooms) / sizeof(classrooms[0]);

typedef struct struct_message {
  char subject[20];
  int  duration;
  int  secondsUntilStart;
  bool startSignal;
  long epochTime;
} struct_message;

typedef struct heartbeat_message {
  bool isHeartbeat;
  long epochTime;
} heartbeat_message;

typedef struct cancel_message {
  bool cancel;
} cancel_message;

struct_message    examData;
heartbeat_message hbData;
cancel_message    cancelData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {}

void loadSentStates() {
  prefs.begin("rooms", true);
  for (int i = 0; i < numSlaves; i++)
    classrooms[i].lastActiveState = prefs.getBool(("sent_" + classrooms[i].roomName).c_str(), false);
  prefs.end();
}

void saveSentState(String roomName, bool state) {
  prefs.begin("rooms", false);
  prefs.putBool(("sent_" + roomName).c_str(), state);
  prefs.end();
}

void setup() {
  Serial.begin(115200);
  loadSentStates();

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(1000); Serial.print("."); }
  Serial.println("\nConnected! Channel: " + String(WiFi.channel()));

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("NTP sync");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) { delay(500); Serial.print("."); }
  Serial.println(" done");

  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW failed"); return; }
  esp_now_register_send_cb(OnDataSent);

  for (int i = 0; i < numSlaves; i++) {
    memcpy(peerInfo.peer_addr, classrooms[i].mac, 6);
    peerInfo.channel = 6;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) == ESP_OK)
      Serial.println("Registered: " + classrooms[i].roomName);
  }
  Serial.println("Ready.");
}

void sendHeartbeat() {
  time_t now; time(&now);
  hbData.isHeartbeat = true;
  hbData.epochTime   = (long)now + 19800;
  for (int i = 0; i < numSlaves; i++)
    esp_now_send(classrooms[i].mac, (uint8_t *)&hbData, sizeof(hbData));
}

void checkFirebase() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(FIREBASE_HOST + "?auth=" + FIREBASE_AUTH);
  int code = http.GET();

  if (code > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    if (!deserializeJson(doc, payload)) {

      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) { http.end(); return; }
      int nowSecs = (timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec + 19800) % 86400;

      time_t now; time(&now);
      long istEpoch = (long)now + 19800;

      for (int i = 0; i < numSlaves; i++) {
        String room = classrooms[i].roomName;
        if (!doc.containsKey(room)) continue;

        bool   isActive = doc[room]["active"];
        String subject  = doc[room]["subject"].as<String>();
        int    tHour    = doc[room]["hour"]     | 0;
        int    tMin     = doc[room]["minute"]   | 0;
        int    duration = doc[room]["duration"] | 120;

        if (isActive && !classrooms[i].lastActiveState) {
          int targetSecs   = tHour * 3600 + tMin * 60;
          int secondsUntil = targetSecs - nowSecs;
          if (secondsUntil < 10)    secondsUntil = 0;
          if (secondsUntil < -3600) secondsUntil += 86400;

          subject.toCharArray(examData.subject, 20);
          examData.duration          = duration;
          examData.secondsUntilStart = secondsUntil;
          examData.startSignal       = true;
          examData.epochTime         = istEpoch;

          esp_err_t res = esp_now_send(classrooms[i].mac, (uint8_t *)&examData, sizeof(examData));
          if (res == ESP_OK) {
            classrooms[i].lastActiveState = true;
            saveSentState(room, true);
            Serial.println("Sent exam to Room " + room);
          } else {
            Serial.println("Failed Room " + room);
          }

        } else if (!isActive && classrooms[i].lastActiveState) {
          // Send cancel packet
          cancelData.cancel = true;
          esp_now_send(classrooms[i].mac, (uint8_t *)&cancelData, sizeof(cancelData));
          classrooms[i].lastActiveState = false;
          saveSentState(room, false);
          Serial.println("Cancelled Room " + room);
        }
      }
    }
  }
  http.end();
}

void loop() {
  static unsigned long lastCheck = 0;
  static unsigned long lastHB    = 0;

  if (millis() - lastHB > 3000) {
    sendHeartbeat();
    lastHB = millis();
  }
  if (millis() - lastCheck > 5000) {
    checkFirebase();
    lastCheck = millis();
  }
}