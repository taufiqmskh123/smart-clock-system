#include <Arduino.h>
#include "ESP32_NOW.h"
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const String myRoom    = "101";           
const char*  ssid      = "-";
const char*  password  = "-";
const int    buzzerPin = 25;

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

struct_message incomingData;

long          baseEpoch   = 0;
unsigned long baseMillis  = 0;
bool          clockSynced = false;

long nowEpoch() {
  return baseEpoch + (long)((millis() - baseMillis) / 1000);
}

void syncClock(long epoch) {
  baseEpoch  = epoch;
  baseMillis = millis();
  clockSynced = true;
}

void epochToHMS(long epoch, int &h, int &m, int &s) {
  long d = epoch % 86400;
  h = d / 3600;
  m = (d % 3600) / 60;
  s = d % 60;
}

enum ExamState { IDLE, COUNTDOWN, BUZZING_START, EXAM_RUNNING, BUZZING_END };
ExamState state = IDLE;

int  countdownSecs = 0;
int  examSecs      = 0;
int  buzzTimer     = 0;
unsigned long lastTick = 0;

void showClock() {
  if (!clockSynced) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(15, 28);
    display.println("WAITING FOR SYNC");
    display.display();
    return;
  }
  int h, m, s;
  epochToHMS(nowEpoch(), h, m, s);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("ROOM "); display.println(myRoom);
  display.drawLine(0, 10, 128, 10, WHITE);
  display.setTextSize(2);
  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  display.setCursor(4, 22);
  display.println(buf);
  display.display();
}

void showCountdown(int secs) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("ROOM "); display.println(myRoom);
  display.drawLine(0, 10, 128, 10, WHITE);
  display.setCursor(0, 14);
  display.print(incomingData.subject);
  display.println(" starts in:");
  display.setTextSize(3);
  display.setCursor(52, 32);
  display.print(secs); display.println("s");
  display.display();
}

void showExamStart() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 10); display.println("EXAM");
  display.setCursor(10, 32); display.println("STARTING!");
  display.display();
}

void showExamTimer(int secs) {
  int mins = secs / 60;
  int s    = secs % 60;
  display.clearDisplay();
  display.setTextSize(3);
  char buf[6];
  sprintf(buf, "%02d:%02d", mins, s);
  display.setCursor(10, 8);
  display.println(buf);
  display.drawLine(0, 40, 128, 40, WHITE);
  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print(incomingData.subject);
  display.print(" | RM "); display.println(myRoom);
  display.setCursor(0, 55);
  display.print(incomingData.duration); display.println(" min exam");
  display.display();
}

void showTimeUp() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 24);
  display.println("TIME'S UP!");
  display.display();
}

void cancelExam() {
  digitalWrite(buzzerPin, LOW);
  state = IDLE;
  showClock();
  Serial.println("Exam cancelled.");
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len == sizeof(cancel_message)) {
    cancel_message cm;
    memcpy(&cm, data, sizeof(cm));
    if (cm.cancel) cancelExam();
    return;
  }
  if (len == sizeof(heartbeat_message)) {
    heartbeat_message hb;
    memcpy(&hb, data, sizeof(hb));
    if (hb.isHeartbeat && state == IDLE) syncClock(hb.epochTime);
    return;
  }
  if (len == sizeof(struct_message)) {
    memcpy(&incomingData, data, sizeof(incomingData));
    if (!incomingData.startSignal) return;
    syncClock(incomingData.epochTime);
    examSecs      = incomingData.duration * 60;
    countdownSecs = incomingData.secondsUntilStart;
    digitalWrite(buzzerPin, HIGH); delay(200); digitalWrite(buzzerPin, LOW);
    if (countdownSecs > 0) {
      state = COUNTDOWN;
    } else {
      state     = BUZZING_START;
      buzzTimer = 10;
    }
    lastTick = millis();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed"); for (;;);
  }
  display.setTextColor(WHITE);
  showClock();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("WiFi connecting");
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
    delay(500); Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    t = millis();
    while (!getLocalTime(&timeinfo) && millis() - t < 5000) delay(500);
    if (getLocalTime(&timeinfo)) {
      time_t now; time(&now);
      syncClock((long)now + 19800);
      Serial.printf("\nIST: %02d:%02d:%02d\n",
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
    WiFi.disconnect(true);
  } else {
    Serial.println("\nWiFi failed, waiting for heartbeat");
  }

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(6);
  while (!WiFi.STA.started()) delay(100);

  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW failed"); return; }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("MAC: " + WiFi.macAddress());
  Serial.println("Ready.");
}

void loop() {
  bool tick = (millis() - lastTick >= 1000);
  if (tick) lastTick = millis();

  switch (state) {
    case IDLE:
      if (tick) showClock();
      break;
    case COUNTDOWN:
      if (tick) {
        showCountdown(countdownSecs);
        countdownSecs--;
        if (countdownSecs <= 0) { state = BUZZING_START; buzzTimer = 10; }
      }
      break;
    case BUZZING_START:
      digitalWrite(buzzerPin, HIGH);
      if (tick) {
        showExamStart();
        buzzTimer--;
        if (buzzTimer <= 0) { digitalWrite(buzzerPin, LOW); state = EXAM_RUNNING; }
      }
      break;
    case EXAM_RUNNING:
      if (tick) {
        showExamTimer(examSecs);
        examSecs--;
        if (examSecs <= 0) { state = BUZZING_END; buzzTimer = 5; }
      }
      break;
    case BUZZING_END:
      digitalWrite(buzzerPin, HIGH);
      if (tick) {
        showTimeUp();
        buzzTimer--;
        if (buzzTimer <= 0) { digitalWrite(buzzerPin, LOW); state = IDLE; showClock(); }
      }
      break;
  }
}