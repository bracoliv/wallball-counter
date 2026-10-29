#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "WallBall-1";
const char* AP_PASS = "12345678";

const int PIN_LEFT  = 34;
const int PIN_RIGHT = 35;
const int THRESHOLD = 800;
const unsigned long DEBOUNCE_MS = 400;

WebServer server(80);

bool pauseLeft = false, pauseRight = false;
unsigned long lastHitLeft = 0, lastHitRight = 0;
unsigned long lastTriggerLeft = 0, lastTriggerRight = 0;
int hitCountLeft = 0, hitCountRight = 0;

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>WallBall</title>
<link rel="manifest" href="/manifest.json">
<meta name="theme-color" content="#0a0a0a">
<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-core"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-converter"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-webgl"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow-models/pose-detection"></script>
<script src="https://cdn.jsdelivr.net/npm/qrcode@1.5.3/build/qrcode.min.js"></script>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0a0a0a;color:#fff;font-family:-apple-system,sans-serif;overflow:hidden}
video,#overlay{position:absolute;top:0;left:0;width:100%;height:100%;object-fit:cover}
#ui{position:absolute;top:0;left:0;width:100%;height:100%;pointer-events:none}
#info{position:absolute;top:12px;left:12px;background:rgba(0,0,0,.7);padding:10px 14px;border-radius:12px;font-size:14px}
#scores{position:absolute;top:16%;left:0;width:100%;display:flex;justify-content:space-around;text-align:center}
.score-box{background:rgba(0,0,0,.55);padding:10px 20px;border-radius:16px;min-width:110px}
.label{font-size:16px;opacity:.8}.num{font-size:64px;font-weight:800;line-height:1}
#time{position:absolute;top:8px;right:12px;font-size:20px;background:rgba(0,0,0,.6);padding:6px 10px;border-radius:10px}
#state{position:absolute;top:52%;left:50%;transform:translateX(-50%);font-size:16px;background:rgba(0,0,0,.55);padding:8px 14px;border-radius:12px}
.ok{color:#4ade80}.bad{color:#f87171}.wait{color:#fbbf24}
#controls{position:absolute;bottom:16px;left:0;width:100%;display:flex;justify-content:center;gap:12px;pointer-events:auto}
button{background:#1f2937;color:#fff;border:none;padding:12px 22px;border-radius:12px;font-size:15px;font-weight:600}
button.active{background:#dc2626}
#qrWrap{position:absolute;bottom:80px;right:12px;background:#fff;padding:6px;border-radius:8px;pointer-events:auto}
#qrWrap canvas{display:block}
#flash{position:absolute;top:0;left:0;width:100%;height:100%;pointer-events:none;opacity:0;transition:opacity .15s}
#flash.green{background:rgba(74,222,128,.35);opacity:1}
#flash.red{background:rgba(248,113,113,.35);opacity:1}
</style>
</head>
<body>
<video id="video" autoplay playsinline muted></video>
<canvas id="overlay"></canvas>
<div id="flash"></div>
<div id="ui">
  <div id="info">正在连接摄像头...</div>
  <div id="time">00:00</div>
  <div id="scores">
    <div class="score-box"><div class="label">女</div><div class="num" id="countLeft">0</div></div>
    <div class="score-box"><div class="label">男</div><div class="num" id="countRight">0</div></div>
  </div>
  <div id="state" class="wait">等待深蹲</div>
  <div id="qrWrap"><canvas id="qr"></canvas></div>
  <div id="controls">
    <button id="btnPause">暂停</button>
    <button id="btnReset">重置</button>
  </div>
</div>
<script>
const ESP_IP=location.hostname||"192.168.4.1";
const video=document.getElementById("video"),overlay=document.getElementById("overlay"),ctx=overlay.getContext("2d");
const info=document.getElementById("info"),countLeftEl=document.getElementById("countLeft"),countRightEl=document.getElementById("countRight");
const timeEl=document.getElementById("time"),stateEl=document.getElementById("state"),flash=document.getElementById("flash"),btnPause=document.getElementById("btnPause");
let detector,running=true,startTime=Date.now(),validLeft=0,validRight=0,lastSeenLeft=0,lastSeenRight=0;
let phase="idle",stoodUpTime=0;const STOODUP_WINDOW=3000;

QRCode.toCanvas(document.getElementById("qr"), "http://"+ESP_IP, {width:90,margin:1});

async function setupCamera(){
  const stream=await navigator.mediaDevices.getUserMedia({video:{facingMode:"user",width:640,height:480},audio:false});
  video.srcObject=stream;
  return new Promise(r=>video.onloadedmetadata=()=>{video.play();overlay.width=video.videoWidth;overlay.height=video.videoHeight;r();});
}
async function loadModel(){
  await tf.setBackend("webgl");await tf.ready();
  detector=await poseDetection.createDetector(poseDetection.SupportedModels.MoveNet,{modelType:poseDetection.movenet.modelType.SINGLEPOSE_LIGHTNING});
  info.textContent="模型已加载";
}
function calcAngle(a,b,c){if(!a||!b||!c)return 180;const rad=Math.atan2(c.y-b.y,c.x-b.x)-Math.atan2(a.y-b.y,a.x-b.x);let ang=Math.abs(rad*180/Math.PI);return ang>180?360-ang:ang;}
function isDeepSquat(kp){
  const lh=kp[11],rh=kp[12],lk=kp[13],rk=kp[14],la=kp[15],ra=kp[16];
  if(!lh||!rh||!lk||!rk||lh.score<0.35||rh.score<0.35)return false;
  const hipY=(lh.y+rh.y)/2,kneeY=(lk.y+rk.y)/2,depthOK=hipY>kneeY+15;
  let angleOK=false;if(la&&ra){const a1=calcAngle(lh,lk,la),a2=calcAngle(rh,rk,ra);angleOK=((a1+a2)/2)<105;}
  return depthOK||angleOK;
}
function isStanding(kp){
  const lh=kp[11],rh=kp[12],lk=kp[13],rk=kp[14];if(!lh||!rh||!lk||!rk)return false;
  return (lh.y+rh.y)/2 < (lk.y+rk.y)/2 - 30;
}
function updatePhase(kp){
  const deep=isDeepSquat(kp),stand=isStanding(kp);
  if(phase==="idle"&&deep)phase="squatted";
  else if(phase==="squatted"&&stand){phase="stoodup";stoodUpTime=Date.now();}
  else if(phase==="stoodup"&&Date.now()-stoodUpTime>STOODUP_WINDOW)phase="idle";
  stateEl.textContent=phase==="idle"?"等待深蹲":phase==="squatted"?"已蹲够，请站起":"已站起，可丢球";
  stateEl.className=phase==="idle"?"wait":"ok";
}
async function pollESP(){
  try{
    const res=await fetch("/status",{cache:"no-store"});
    const data=await res.json();
    info.textContent="已连接 ESP";
    if((data.lastLeft||0)>lastSeenLeft){
      lastSeenLeft=data.lastLeft;
      if(phase==="stoodup"){validLeft++;countLeftEl.textContent=validLeft;flash.className="green";setTimeout(()=>flash.className="",180);phase="idle";}
      else{flash.className="red";setTimeout(()=>flash.className="",180);}
    }
    if((data.lastRight||0)>lastSeenRight){
      lastSeenRight=data.lastRight;
      if(phase==="stoodup"){validRight++;countRightEl.textContent=validRight;flash.className="green";setTimeout(()=>flash.className="",180);phase="idle";}
      else{flash.className="red";setTimeout(()=>flash.className="",180);}
    }
  }catch(e){info.textContent="无法连接";}
}
function updateTimer(){
  if(!running)return;const s=Math.floor((Date.now()-startTime)/1000);
  timeEl.textContent=String(Math.floor(s/60)).padStart(2,"0")+":"+String(s%60).padStart(2,"0");
}
async function detectLoop(){
  if(!detector||!running){requestAnimationFrame(detectLoop);return;}
  const poses=await detector.estimatePoses(video,{flipHorizontal:false});
  ctx.clearRect(0,0,overlay.width,overlay.height);
  ctx.save();ctx.scale(-1,1);ctx.drawImage(video,-overlay.width,0,overlay.width,overlay.height);ctx.restore();
  if(poses.length){
    const kp=poses[0].keypoints;updatePhase(kp);
    [11,12,13,14,15,16].forEach(i=>{const p=kp[i];if(p&&p.score>0.3){ctx.beginPath();ctx.arc(overlay.width-p.x,p.y,5,0,Math.PI*2);ctx.fillStyle=phase==="stoodup"?"#4ade80":"#60a5fa";ctx.fill();}});
  }
  requestAnimationFrame(detectLoop);
}
btnPause.onclick=()=>{running=!running;btnPause.textContent=running?"暂停":"继续";btnPause.classList.toggle("active",!running);if(running)startTime=Date.now()-(Date.now()-startTime);};
document.getElementById("btnReset").onclick=()=>{validLeft=validRight=0;countLeftEl.textContent=countRightEl.textContent="0";phase="idle";startTime=Date.now();lastSeenLeft=lastSeenRight=0;};
async function main(){await setupCamera();await loadModel();setInterval(pollESP,150);setInterval(updateTimer,200);detectLoop();}
main();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleManifest() {
  server.send(200, "application/json",
    "{\"name\":\"WallBall\",\"short_name\":\"WallBall\",\"start_url\":\"/\",\"display\":\"standalone\",\"background_color\":\"#0a0a0a\",\"theme_color\":\"#0a0a0a\"}");
}

void handleStatus() {
  String json = "{\"left\":" + String(hitCountLeft) +
                ",\"right\":" + String(hitCountRight) +
                ",\"lastLeft\":" + String(lastHitLeft) +
                ",\"lastRight\":" + String(lastHitRight) +
                ",\"pauseLeft\":" + String(pauseLeft ? "true" : "false") +
                ",\"pauseRight\":" + String(pauseRight ? "true" : "false") + "}";
  sendCors();
  server.send(200, "application/json", json);
}

void handlePauseLeft() { pauseLeft = !pauseLeft; sendCors(); server.send(200, "text/plain", pauseLeft ? "paused" : "resumed"); }
void handlePauseRight() { pauseRight = !pauseRight; sendCors(); server.send(200, "text/plain", pauseRight ? "paused" : "resumed"); }

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LEFT, INPUT);
  pinMode(PIN_RIGHT, INPUT);

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/manifest.json", handleManifest);
  server.on("/status", handleStatus);
  server.on("/pauseLeft", handlePauseLeft);
  server.on("/pauseRight", handlePauseRight);
  server.begin();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  if (!pauseLeft) {
    int val = analogRead(PIN_LEFT);
    if (val > THRESHOLD && (now - lastTriggerLeft) > DEBOUNCE_MS) {
      lastTriggerLeft = now; lastHitLeft = now; hitCountLeft++;
    }
  }
  if (!pauseRight) {
    int val = analogRead(PIN_RIGHT);
    if (val > THRESHOLD && (now - lastTriggerRight) > DEBOUNCE_MS) {
      lastTriggerRight = now; lastHitRight = now; hitCountRight++;
    }
  }
}
