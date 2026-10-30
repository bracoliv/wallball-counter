#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "WallBall-1";
const char* AP_PASS = "12345678";

const int PIN_LEFT  = 34;   // BOTTOM / 女
const int PIN_RIGHT = 35;   // TOP / 男

int thresholdLeft  = 800;
int thresholdRight = 800;
const unsigned long DEBOUNCE_MS = 400;

WebServer server(80);

unsigned long lastHitLeft = 0, lastHitRight = 0;
unsigned long lastTriggerLeft = 0, lastTriggerRight = 0;
int hitCountLeft = 0, hitCountRight = 0;
int lastAdcLeft = 0, lastAdcRight = 0;

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
}

void handleStatus() {
  int adcL = analogRead(PIN_LEFT);
  int adcR = analogRead(PIN_RIGHT);
  String json = "{\"left\":" + String(hitCountLeft) +
                ",\"right\":" + String(hitCountRight) +
                ",\"lastLeft\":" + String(lastHitLeft) +
                ",\"lastRight\":" + String(lastHitRight) +
                ",\"adcLeft\":" + String(adcL) +
                ",\"adcRight\":" + String(adcR) +
                ",\"lastAdcLeft\":" + String(lastAdcLeft) +
                ",\"lastAdcRight\":" + String(lastAdcRight) +
                ",\"threshLeft\":" + String(thresholdLeft) +
                ",\"threshRight\":" + String(thresholdRight) + "}";
  sendCors();
  server.send(200, "application/json", json);
}

void handleSet() {
  if (server.hasArg("left")) {
    int v = server.arg("left").toInt();
    if (v >= 100 && v <= 3000) thresholdLeft = v;
  }
  if (server.hasArg("right")) {
    int v = server.arg("right").toInt();
    if (v >= 100 && v <= 3000) thresholdRight = v;
  }
  sendCors();
  server.send(200, "text/plain", "ok");
  Serial.printf("Threshold set: LEFT=%d RIGHT=%d\n", thresholdLeft, thresholdRight);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LEFT, INPUT);
  pinMode(PIN_RIGHT, INPUT);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/status", handleStatus);
  server.on("/set", handleSet);
  server.begin();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  int valL = analogRead(PIN_LEFT);
  if (valL > thresholdLeft && (now - lastTriggerLeft) > DEBOUNCE_MS) {
    lastTriggerLeft = now;
    lastHitLeft = now;
    lastAdcLeft = valL;
    hitCountLeft++;
    Serial.printf("LEFT HIT | ADC=%d | Count=%d | Thresh=%d\n", valL, hitCountLeft, thresholdLeft);
  }

  int valR = analogRead(PIN_RIGHT);
  if (valR > thresholdRight && (now - lastTriggerRight) > DEBOUNCE_MS) {
    lastTriggerRight = now;
    lastHitRight = now;
    lastAdcRight = valR;
    hitCountRight++;
    Serial.printf("RIGHT HIT | ADC=%d | Count=%d | Thresh=%d\n", valR, hitCountRight, thresholdRight);
  }
}
