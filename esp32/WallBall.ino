#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "WallBall-1";
const char* AP_PASS = "12345678";

const int PIN_LEFT  = 34;   // 女板
const int PIN_RIGHT = 35;   // 男板

const int THRESHOLD = 800;
const unsigned long DEBOUNCE_MS = 400;

WebServer server(80);

bool pauseLeft  = false;
bool pauseRight = false;

unsigned long lastHitLeft  = 0;
unsigned long lastHitRight = 0;
unsigned long lastTriggerLeft  = 0;
unsigned long lastTriggerRight = 0;

int hitCountLeft  = 0;
int hitCountRight = 0;

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
  server.sendHeader("Cache-Control", "no-store");
}

void handleStatus() {
  String json = "{";
  json += "\"left\":"  + String(hitCountLeft)  + ",";
  json += "\"right\":" + String(hitCountRight) + ",";
  json += "\"lastLeft\":"  + String(lastHitLeft)  + ",";
  json += "\"lastRight\":" + String(lastHitRight) + ",";
  json += "\"pauseLeft\":"  + String(pauseLeft ? "true" : "false") + ",";
  json += "\"pauseRight\":" + String(pauseRight ? "true" : "false");
  json += "}";
  sendCors();
  server.send(200, "application/json", json);
}

void handleOptions() {
  sendCors();
  server.send(204);
}

void handlePauseLeft() {
  pauseLeft = !pauseLeft;
  sendCors();
  server.send(200, "text/plain", pauseLeft ? "paused" : "resumed");
}

void handlePauseRight() {
  pauseRight = !pauseRight;
  sendCors();
  server.send(200, "text/plain", pauseRight ? "paused" : "resumed");
}

void handleRoot() {
  server.send(200, "text/html",
    "<h2>WallBall ESP32</h2>"
    "<p>女 (Left): " + String(hitCountLeft) + "</p>"
    "<p>男 (Right): " + String(hitCountRight) + "</p>"
    "<p><a href='/status'>JSON Status</a></p>");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LEFT, INPUT);
  pinMode(PIN_RIGHT, INPUT);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/pauseLeft", handlePauseLeft);
  server.on("/pauseRight", handlePauseRight);

  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (!pauseLeft) {
    int val = analogRead(PIN_LEFT);
    if (val > THRESHOLD && (now - lastTriggerLeft) > DEBOUNCE_MS) {
      lastTriggerLeft = now;
      lastHitLeft = now;
      hitCountLeft++;
      Serial.println("Left HIT: " + String(val));
    }
  }

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
