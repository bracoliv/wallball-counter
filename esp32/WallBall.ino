#include <WiFi.h>
#include <WebServer.h>
// ================== 配置 ==================
const char* AP_SSID = "WallBall-1";
const char* AP_PASS = "12345678";
const int PIN_LEFT  = 34;   // 左侧/女板 压电片
const int PIN_RIGHT = 35;   // 右侧/男板 压电片
const int THRESHOLD = 800;  // 撞击阈值（需根据实际调整）
const unsigned long DEBOUNCE_MS = 400;
// ================== 全局变量 ==================
WebServer server(80);
bool pauseLeft  = false;
bool pauseRight = false;
unsigned long lastHitLeft  = 0;
unsigned long lastHitRight = 0;
unsigned long lastTriggerLeft  = 0;
unsigned long lastTriggerRight = 0;
int hitCountLeft  = 0;
int hitCountRight = 0;
// ================== 接口 ==================
void handleStatus() {
  String json = "{";
  json += "\"left\":"  + String(hitCountLeft)  + ",";
  json += "\"right\":" + String(hitCountRight) + ",";
  json += "\"lastLeft\":"  + String(lastHitLeft)  + ",";
  json += "\"lastRight\":" + String(lastHitRight) + ",";
  json += "\"pauseLeft\":"  + String(pauseLeft ? "true" : "false") + ",";
  json += "\"pauseRight\":" + String(pauseRight ? "true" : "false");
  json += "}";
  // 允许现有 GitHub Pages 网页读取状态，且不缓存撞击数据。
  server.sendHeader("Access-Control-Allow-Origin", "https://bracoliv.github.io");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}
void handlePauseLeft() {
  pauseLeft = !pauseLeft;
  server.send(200, "text/plain", pauseLeft ? "paused" : "resumed");
}
void handlePauseRight() {
  pauseRight = !pauseRight;
  server.send(200, "text/plain", pauseRight ? "paused" : "resumed");
}
void handleRoot() {
  server.send(200, "text/html",
    "<h2>WallBall ESP32</h2>"
    "<p>Left: " + String(hitCountLeft) + "</p>"
    "<p>Right: " + String(hitCountRight) + "</p>"
    "<p><a href='/status'>JSON Status</a></p>");
}
// ================== Setup ==================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LEFT, INPUT);
  pinMode(PIN_RIGHT, INPUT);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/pauseLeft", handlePauseLeft);
  server.on("/pauseRight", handlePauseRight);
  server.begin();
  Serial.println("Server started");
}
// ================== Loop ==================
void loop() {
  server.handleClient();
  unsigned long now = millis();
  // 左传感器
  if (!pauseLeft) {
    int val = analogRead(PIN_LEFT);
    if (val > THRESHOLD && (now - lastTriggerLeft) > DEBOUNCE_MS) {
      lastTriggerLeft = now;
      lastHitLeft = now;
      hitCountLeft++;
      Serial.println("Left HIT: " + String(val));
    }
  }
  // 右传感器
  if (!pauseRight) {
    int val = analogRead(PIN_RIGHT);
    if (val > THRESHOLD && (now - lastTriggerRight) > DEBOUNCE_MS) {
      lastTriggerRight = now;
      lastHitRight = now;
      hitCountRight++;
      Serial.println("Right HIT: " + String(val));
    }
  }
}
