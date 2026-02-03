#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include <math.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Preferences.h>
#include "boot_sequence.h"

Preferences prefs;

// ==================================================
// 1. HARDWARE CONFIG
// ==================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 20
#define SCL_PIN 21
#define TOUCH_PIN 1
#define BUZZER_PIN 2

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool timeConfigured = false;

// ==================================================
// 2. PHYSICS ENGINE STRUCT (MOVED TO TOP FOR FIX)
// ==================================================
struct Eye {
  float x, y, w, h;
  float targetX, targetY, targetW, targetH;
  float pupilX, pupilY, targetPupilX, targetPupilY;
  float velX, velY, velW, velH, pVelX, pVelY;
  float k = 0.22, d = 0.55, pk = 0.15, pd = 0.50;
  bool blinking;
  unsigned long lastBlink, nextBlinkTime;

  void init(float _x, float _y, float _w, float _h) {
    x = targetX = _x; y = targetY = _y;
    w = targetW = _w; h = targetH = _h;
    pupilX = targetPupilX = 0; pupilY = targetPupilY = 0;
    nextBlinkTime = millis() + random(1000, 4000);
  }

  void update() {
    float ax = (targetX - x) * k; float ay = (targetY - y) * k;
    float aw = (targetW - w) * k; float ah = (targetH - h) * k;
    velX = (velX + ax) * d; velY = (velY + ay) * d;
    velW = (velW + aw) * d; velH = (velH + ah) * d;
    x += velX; y += velY; w += velW; h += velH;
    float pax = (targetPupilX - pupilX) * pk; float pay = (targetPupilY - pupilY) * pk;
    pVelX = (pVelX + pax) * pd; pVelY = (pVelY + pay) * pd;
    pupilX += pVelX; pupilY += pVelY;
  }
};

Eye leftEye, rightEye; // Global instances

// ==================================================
// 3. SETTINGS & VARIABLES
// ==================================================
String serialBuffer = "";
bool webOverrideEyes = false;
unsigned long webEyeTimeout = 0;

// EYES PERSISTENT
float storedEyeW = 36; float storedEyeH = 36;
int storedEyeR = 8; float storedEyeX = 0; float storedEyeY = 0;

// BOOT TEXT
String bootLine1 = "Booting"; String bootLine2 = "BOBO 2.2";
String bootLine3 = "Loading..."; String bootLine4 = "Here We Go";

// FLAGS
bool soundEnabled = true;
bool chimeEnabled = true; 
bool waterEnabled = false; 
int timerMinutes = 10;    

// ALARM VARIABLES
int alarmHour = 7;
int alarmMin = 0;
bool alarmOn = false;
bool alarmTriggered = false; 
bool isAlarmRinging = false; 

// TIMERS
unsigned long lastWeatherUpdate = 0;
unsigned long weatherInterval = 7200000; // 2 HOURS
unsigned long lastPageSwitch = 0;
unsigned long currentPageInterval = 25000;
unsigned long lastWaterAlert = 0;
const unsigned long WATER_INTERVAL = 3600000; // 1 Hour
unsigned long lastHeavyTaskTime = 0; 
unsigned long lastInteraction = 0; 
const unsigned long SLEEP_TIMEOUT = 7200000; // 2 Hours

// COUNTDOWN
bool isCountingDown = false;
unsigned long countdownStart = 0;
unsigned long countdownDuration = 0;

// SLEEP / LOW POWER STATE
bool isSleeping = false;
bool isLowPowerMode = false;

// UI PAGES
// 0: Eyes, 1: Time, 2: Weather, 3: WorldClock, 4: Forecast, 5: AlarmView, 6: AlarmSet
int currentPage = 0; 
bool highBrightness = true;

// EDITING STATE
int editField = 0; // 0:Hour, 1:Min, 2:AMPM, 3:Toggle
unsigned long lastBlinkTime = 0;
bool blinkState = true;

// INPUT STATE
int tapCounter = 0;
unsigned long lastTapTime = 0;
bool lastPinState = false;
unsigned long pressStartTime = 0;
bool isLongPressHandled = false;
const unsigned long LONG_PRESS_TIME = 800;
const unsigned long DOUBLE_TAP_DELAY = 450; 

// CONSTANTS
const long DURATION_EYES = 25000;
const long DURATION_INFO = 5000;

// WIFI & WEATHER
bool wifiEnabled = false;
String wifiSSID = ""; String wifiPASS = "";
unsigned long lastWifiAttempt = 0;
String city = ""; const char* countryCode = "IN"; String apiKey = "";
float temperature = 0.0; int humidity = 0;
String weatherMain = "Offline"; String weatherDesc = "No WiFi";

// TIMEZONES
long zone1Offset = 0; String zone1Name = "UTC";
long zone2Offset = 32400; String zone2Name = "JAPAN";

struct ForecastDay { String dayName; int temp; String iconType; };
ForecastDay fcast[3];

const char* ntpServer = "pool.ntp.org";
const char* tzString = "IST-5:30";

#define MOOD_NORMAL 0
#define MOOD_HAPPY 1
#define MOOD_SURPRISED 2
#define MOOD_SLEEPY 3
#define MOOD_ANGRY 4
#define MOOD_SAD 5
#define MOOD_EXCITED 6
#define MOOD_LOVE 7
#define MOOD_SUSPICIOUS 8
int currentMood = MOOD_NORMAL;

// PHYSICS VARS
unsigned long lastSaccade = 0;
unsigned long saccadeInterval = 1000; 
float breathVal = 0;

// ==================================================
// ICONS (Must be defined before usage)
// ==================================================
const unsigned char bmp_clear[] PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0xc0,0x80,0x00,0x0f,0xf0,0x00,0x00,0x3f,0xfc,0x00,0x00,0x7f,0xfe,0x00,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x00,0x7f,0xfe,0x00,0x00,0x3f,0xfc,0x00,0x01,0x0f,0xf0,0x80,0x00,0x03,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_clouds[] PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x3f,0xff,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x00,0xff,0xff,0xe0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_rain[] PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x0c,0x00,0x00,0x60,0x0c,0x00,0x00,0xe0,0x1c,0x00,0x00,0xc0,0x18,0x00,0x03,0x80,0x70,0x00,0x03,0x80,0x70,0x00,0x03,0x00,0x60,0x00,0x02,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char mini_sun[] PROGMEM = { 0x00,0x00,0x01,0x80,0x00,0x00,0x10,0x08,0x04,0x20,0x03,0xc0,0x27,0xe4,0x07,0xe0,0x07,0xe0,0x27,0xe4,0x03,0xc0,0x04,0x20,0x10,0x08,0x00,0x00,0x01,0x80,0x00,0x00 };
const unsigned char mini_cloud[] PROGMEM = { 0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x0f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char mini_rain[] PROGMEM = { 0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x00,0x00,0x44,0x44,0x22,0x22,0x11,0x11 };
const unsigned char bmp_tiny_drop[] PROGMEM = { 0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x7c, 0x38, 0x00 };

const unsigned char* getBigIcon(String w) {
  if (w == "Clear") return bmp_clear;
  if (w == "Clouds") return bmp_clouds;
  if (w == "Rain" || w == "Drizzle") return bmp_rain;
  return bmp_clouds;
}

const unsigned char* getMiniIcon(String w) {
  if (w == "Clear") return mini_sun;
  if (w == "Rain" || w == "Drizzle" || w == "Thunderstorm") return mini_rain;
  return mini_cloud;
}

void getWeatherAndForecast(); // Forward declaration

// ==================================================
// AUDIO
// ==================================================
void beep(int freq, int duration) {
  if (!soundEnabled) return;
  ledcAttach(BUZZER_PIN, freq, 10); 
  ledcWrite(BUZZER_PIN, 512); delay(duration); 
  ledcWrite(BUZZER_PIN, 0); ledcDetach(BUZZER_PIN);           
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
}
void beepClick() { beep(2000, 20); } 
void beepAlert() { beep(1500,80); delay(40); beep(1500,80); }

void playStartupSound() {
  if (!soundEnabled) return;
  int melody[] = { 622, 932, 1244, 1864, 1244, 1864 }; 
  int duration[] = { 150, 150, 150, 400, 150, 400 };
  for (int i = 0; i < 6; i++) { beep(melody[i], duration[i]); delay(20); }
}

void playAlarmSound() {
  if (!soundEnabled) return;
  beep(2000, 100); delay(50); beep(1500, 100); delay(50);
  beep(2000, 100); delay(50); beep(1500, 100); delay(50);
}

void playMoodSound(int mood) {
  if (!soundEnabled) return;
  switch(mood) {
    case MOOD_HAPPY: beep(1046, 80); delay(30); beep(1318, 80); delay(30); beep(1568, 120); break;
    case MOOD_SAD: beep(100, 100); delay(50); beep(100, 300); break; 
    case MOOD_ANGRY: beep(880, 150); delay(20); beep(830, 150); delay(20); beep(784, 300); break; 
    case MOOD_SURPRISED: beep(1000, 50); beep(2000, 150); break;
    case MOOD_SLEEPY: beep(600, 300); delay(100); beep(400, 400); break;
    case MOOD_LOVE: beep(1500, 150); delay(50); beep(1200, 300); break;
    default: beepClick(); break;
  }
}

// ==================================================
// WEB & DATA
// ==================================================
void getForecast() {
  if (!wifiEnabled || WiFi.status() != WL_CONNECTED || apiKey == "") return;
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + countryCode + "&appid=" + apiKey + "&units=metric&cnt=25";
  http.begin(url);
  if (http.GET() == 200) {
    JSONVar o = JSON.parse(http.getString());
    int indices[3] = {7, 15, 23};
    for (int i = 0; i < 3; i++) {
      int idx = indices[i];
      fcast[i].temp = int(o["list"][idx]["main"]["temp"]);
      fcast[i].iconType = (const char*)o["list"][idx]["weather"][0]["main"];
      long dt = (long)o["list"][idx]["dt"];
      time_t t = dt; struct tm* timeInfo = localtime(&t);
      char dayBuf[4]; strftime(dayBuf, 4, "%a", timeInfo); 
      fcast[i].dayName = String(dayBuf);
    }
  } 
  http.end();
}

void getWeatherAndForecast() {
  if (!wifiEnabled || WiFi.status() != WL_CONNECTED || apiKey == "") return;
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&appid=" + apiKey + "&units=metric";
  http.begin(url);
  if (http.GET() == 200) {
    JSONVar o = JSON.parse(http.getString());
    temperature = double(o["main"]["temp"]);
    humidity = int(o["main"]["humidity"]);
    weatherMain = (const char*)o["weather"][0]["main"];
    weatherDesc = (const char*)o["weather"][0]["description"];
    weatherDesc[0] = toupper(weatherDesc[0]);
  }
  http.end();
  getForecast();
}

void sendSettings() {
  Serial.println("SET:WIFI_SSID=" + wifiSSID);
  Serial.println("SET:WIFI_PASS=" + wifiPASS);
  Serial.println("SET:CITY=" + city);
  Serial.println("SET:APIKEY=" + apiKey);
  Serial.println("SET:Z1=" + zone1Name + "," + String(zone1Offset));
  Serial.println("SET:Z2=" + zone2Name + "," + String(zone2Offset));
  Serial.println("SET:ALARM=" + String(alarmHour) + "," + String(alarmMin) + "," + String(alarmOn));
  Serial.println("SET:TIMER=" + String(timerMinutes));
  Serial.println("SET:SND=" + String(soundEnabled));
  Serial.println("SET:CHM=" + String(chimeEnabled));
  Serial.println("SET:WTR=" + String(waterEnabled));
  Serial.println("SET:BT1=" + bootLine1);
  Serial.println("SET:BT2=" + bootLine2);
  Serial.println("SET:BT3=" + bootLine3);
  Serial.println("SET:BT4=" + bootLine4);
  Serial.println("SET:EYE=" + String(storedEyeX) + "," + String(storedEyeY) + "," + String(storedEyeW) + "," + String(storedEyeH) + "," + String(storedEyeR));
  Serial.println("SET:DONE");
}

void handleSerialCommand(String cmd) {
  cmd.trim(); if (cmd.length() == 0) return;
  lastInteraction = millis();
  
  if (isLowPowerMode) {
      isLowPowerMode = false;
      display.ssd1306_command(SSD1306_SETCONTRAST); display.ssd1306_command(255); 
      currentMood = MOOD_NORMAL;
  }

  int sep = cmd.indexOf(':');
  String key, val;
  if (sep >= 0) { key = cmd.substring(0, sep); val = cmd.substring(sep + 1); } 
  else { key = cmd; val = ""; }
  key.toUpperCase();

  if (key == "REBOOT") { ESP.restart(); }
  else if (key == "GET_SETTINGS") { sendSettings(); }
  else if (key == "SOUND") { soundEnabled = (val == "1"); }
  else if (key == "CHIME") { chimeEnabled = (val == "1"); }
  else if (key == "WATER") { waterEnabled = (val == "1"); lastWaterAlert = millis(); }
  else if (key == "TIMER_SET") { timerMinutes = val.toInt(); if(timerMinutes<1) timerMinutes=1; }
  else if (key == "ALARM_SET") {
      int c1 = val.indexOf(','); int c2 = val.lastIndexOf(',');
      if(c1 > 0 && c2 > 0) {
          alarmHour = val.substring(0, c1).toInt();
          alarmMin = val.substring(c1+1, c2).toInt();
          alarmOn = (val.substring(c2+1).toInt() == 1);
          Serial.println("OK ALARM");
      }
  }
  else if (key == "BOOT1") bootLine1 = val;
  else if (key == "BOOT2") bootLine2 = val;
  else if (key == "BOOT3") bootLine3 = val;
  else if (key == "BOOT4") bootLine4 = val;
  else if (key == "WIFI_SSID") { wifiSSID = val; wifiEnabled = true; }
  else if (key == "WIFI_PASS") { wifiPASS = val; }
  else if (key == "WIFI_ON") { wifiEnabled = true; lastWifiAttempt = 0; }
  else if (key == "WIFI_OFF") { wifiEnabled = false; WiFi.disconnect(true); timeConfigured = false; }
  else if (key == "CITY") { city = val; getWeatherAndForecast(); }
  else if (key == "APIKEY") { apiKey = val; getWeatherAndForecast(); }
  else if (key == "MOOD") { int m = val.toInt(); if (m >= 0 && m <= MOOD_SUSPICIOUS) { currentMood = m; lastSaccade = 0; playMoodSound(currentMood); } }
  else if (key == "PAGE") { int p = val.toInt(); if (p >= 0 && p <= 6) { currentPage = p; lastPageSwitch = millis(); } }
  else if (key == "BRIGHT") { 
      int b = constrain(val.toInt(), 1, 255); 
      display.ssd1306_command(SSD1306_SETCONTRAST); display.ssd1306_command(b); 
      highBrightness = (b > 100);
  }
  else if (key == "EYE") {
    int c = val.indexOf(',');
    if (c > 0) {
      float x = constrain(val.substring(0, c).toFloat(), -10, 10);
      float y = constrain(val.substring(c + 1).toFloat(), -10, 10);
      leftEye.targetPupilX = x; leftEye.targetPupilY = y;
      rightEye.targetPupilX = x; rightEye.targetPupilY = y;
      webOverrideEyes = true; webEyeTimeout = millis();
    }
  }
  else if (key == "EYE_POS") { int c = val.indexOf(','); if (c > 0) { storedEyeX = val.substring(0, c).toFloat(); storedEyeY = val.substring(c + 1).toFloat(); } }
  else if (key == "EYE_SIZE") { int c = val.indexOf(','); if (c > 0) { storedEyeW = constrain(val.substring(0, c).toFloat(), 10, 60); storedEyeH = constrain(val.substring(c + 1).toFloat(), 10, 60); } }
  else if (key == "EYE_RADIUS") { storedEyeR = constrain(val.toInt(), 0, 20); }
  else if (key == "EYE_SAVE") {
    prefs.putFloat("eW", storedEyeW); prefs.putFloat("eH", storedEyeH);
    prefs.putInt("eR", storedEyeR); prefs.putFloat("eX", storedEyeX);
    prefs.putFloat("eY", storedEyeY); 
  }
  else if (key == "Z1_SET") { int c = val.indexOf(','); if (c > 0) { zone1Name = val.substring(0, c); zone1Offset = val.substring(c + 1).toInt(); } }
  else if (key == "Z2_SET") { int c = val.indexOf(','); if (c > 0) { zone2Name = val.substring(0, c); zone2Offset = val.substring(c + 1).toInt(); } }
  else if (key == "SAVE") {
    prefs.putString("ssid", wifiSSID); prefs.putString("pass", wifiPASS);
    prefs.putString("city", city); prefs.putString("api", apiKey);
    prefs.putString("z1n", zone1Name); prefs.putLong("z1o", zone1Offset);
    prefs.putString("z2n", zone2Name); prefs.putLong("z2o", zone2Offset);
    prefs.putBool("snd", soundEnabled); prefs.putBool("chm", chimeEnabled);
    prefs.putBool("wtr", waterEnabled); prefs.putInt("tmr", timerMinutes);
    prefs.putString("bt1", bootLine1); prefs.putString("bt2", bootLine2);
    prefs.putString("bt3", bootLine3); prefs.putString("bt4", bootLine4);
    prefs.putInt("aH", alarmHour); prefs.putInt("aM", alarmMin); prefs.putBool("aOn", alarmOn);
    prefs.putFloat("eW", storedEyeW); prefs.putFloat("eH", storedEyeH);
    prefs.putInt("eR", storedEyeR); prefs.putFloat("eX", storedEyeX);
    prefs.putFloat("eY", storedEyeY); 
    if(wifiEnabled) { WiFi.disconnect(); lastWifiAttempt = 0; }
  }
}

void checkConnections() {
  if (!wifiEnabled) return;
  if (WiFi.status() == WL_CONNECTED) {
    if (!timeConfigured) {
      configTime(0, 0, ntpServer); setenv("TZ", tzString, 1); tzset();
      timeConfigured = true; getWeatherAndForecast();
    }
  } else {
    if (wifiSSID.length() > 0 && millis() - lastWifiAttempt > 15000) {
      lastWifiAttempt = millis(); WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
    }
  }
}

// ==================================================
// PHYSICS ENGINE
// ==================================================
void updatePhysicsAndMood() {
  unsigned long now = millis();
  breathVal = sin(now / 800.0) * 1.5;

  if (now > leftEye.nextBlinkTime) {
    leftEye.blinking = true; rightEye.blinking = true;
    leftEye.lastBlink = now; leftEye.nextBlinkTime = now + random(2000, 6000);
  }
  if (leftEye.blinking) {
    leftEye.targetH = 2; rightEye.targetH = 2;
    if (now - leftEye.lastBlink > 120) { leftEye.blinking = false; rightEye.blinking = false; }
  }

  if (!leftEye.blinking && !webOverrideEyes && now - lastSaccade > saccadeInterval) {
    lastSaccade = now; saccadeInterval = random(100, 2000); 
    float lx = random(-8, 9); float ly = random(-4, 5);
    leftEye.targetPupilX = lx; leftEye.targetPupilY = ly;
    rightEye.targetPupilX = lx; rightEye.targetPupilY = ly;
    leftEye.targetX = 18 + storedEyeX + lx * 0.3; leftEye.targetY = 14 + storedEyeY + ly * 0.3;
    rightEye.targetX = 74 + storedEyeX + lx * 0.3; rightEye.targetY = 14 + storedEyeY + ly * 0.3;
  }

  if (!leftEye.blinking) {
    float baseW = storedEyeW; float baseH = storedEyeH + breathVal; 
    
    if (isLowPowerMode) {
        currentMood = MOOD_SLEEPY;
        baseH = 4; 
    }

    switch (currentMood) {
      case MOOD_HAPPY: case MOOD_LOVE: baseW = 40; baseH = 32; break;
      case MOOD_SURPRISED: baseW = 30; baseH = 45; break;
      case MOOD_SLEEPY: baseW = 38; baseH = isLowPowerMode ? 4 : 30; break;
      case MOOD_ANGRY: baseW = 34; baseH = 32; break;
      case MOOD_SAD: baseW = 34; baseH = 40; break;
      case MOOD_SUSPICIOUS: leftEye.targetH = 20; rightEye.targetH = 42; break;
    }
    leftEye.targetW = baseW; leftEye.targetH = baseH;
    rightEye.targetW = baseW; rightEye.targetH = baseH;
    if(currentMood == MOOD_SUSPICIOUS) { leftEye.targetH = 20; rightEye.targetH = 42; }
  }
  leftEye.update(); rightEye.update();
  if (webOverrideEyes && millis() - webEyeTimeout > 2000) webOverrideEyes = false;
}

// ==================================================
// DRAWING
// ==================================================
void drawEyelidMask(float x, float y, float w, float h, int mood, bool isLeft) {
  int ix = (int)x; int iy = (int)y; int iw = (int)w; int ih = (int)h;
  if (mood == MOOD_ANGRY) {
    if (isLeft) for (int i=0; i<16; i++) display.drawLine(ix, iy+i, ix+iw, iy-6+i, SSD1306_BLACK);
    else for (int i=0; i<16; i++) display.drawLine(ix, iy-6+i, ix+iw, iy+i, SSD1306_BLACK);
  }
  else if (mood == MOOD_SAD) {
    if (isLeft) for (int i=0; i<16; i++) display.drawLine(ix, iy-6+i, ix+iw, iy+i, SSD1306_BLACK);
    else for (int i=0; i<16; i++) display.drawLine(ix, iy+i, ix+iw, iy-6+i, SSD1306_BLACK);
  }
  else if (mood == MOOD_HAPPY || mood == MOOD_LOVE || mood == MOOD_EXCITED) {
    display.fillRect(ix, iy+ih-12, iw, 14, SSD1306_BLACK);
    display.fillCircle(ix+iw/2, iy+ih+6, iw/1.3, SSD1306_BLACK);
  }
  else if (mood == MOOD_SLEEPY) display.fillRect(ix, iy, iw, ih/2+2, SSD1306_BLACK);
  else if (mood == MOOD_SUSPICIOUS) {
    if (isLeft) display.fillRect(ix, iy, iw, ih/2-2, SSD1306_BLACK);
    else display.fillRect(ix, iy+ih-8, iw, 8, SSD1306_BLACK);
  }
}

void drawUltraProEye(Eye &e, bool isLeft) {
  int ix = (int)e.x; int iy = (int)e.y; int iw = (int)e.w; int ih = (int)e.h;
  int r = storedEyeR; if (r > iw/2) r = iw/2; if (r > ih/2) r = ih/2; if (r < 0) r = 0;
  display.fillRoundRect(ix, iy, iw, ih, r, SSD1306_WHITE);
  int cx = ix + iw / 2; int cy = iy + ih / 2;
  int pw = iw / 2.2; int ph = ih / 2.2;
  int px = cx + (int)e.pupilX - pw / 2;
  int py = cy + (int)e.pupilY - ph / 2;
  px = constrain(px, ix, ix + iw - pw); py = constrain(py, iy, iy + ih - ph);
  display.fillRoundRect(px, py, pw, ph, r / 2, SSD1306_BLACK);
  if (iw > 15 && ih > 15) display.fillCircle(px + pw - 4, py + 4, 2, SSD1306_WHITE);
  drawEyelidMask(e.x, e.y, e.w, e.h, currentMood, isLeft);
}

void drawEmoPage() {
  if(isLowPowerMode) {
      // Rounded Closed Eyes (Thick Lines)
      display.fillRoundRect(22, 30, 32, 6, 2, SSD1306_WHITE); 
      display.fillRoundRect(74, 30, 32, 6, 2, SSD1306_WHITE); 
      
      display.setCursor(110, 10); display.setFont(NULL); display.print("z");
      display.setCursor(116, 4); display.print("Z");
  } else {
      updatePhysicsAndMood();
      drawUltraProEye(leftEye, true);
      drawUltraProEye(rightEye, false);
  }
}

void drawForecastPage() {
  display.fillRect(0,0,128,16,SSD1306_WHITE);
  display.setFont(NULL); display.setTextColor(SSD1306_BLACK);
  display.setCursor(20,4); display.print("3-DAY FORECAST");
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(42,16,42,64,SSD1306_WHITE); display.drawLine(85,16,85,64,SSD1306_WHITE);
  for(int i=0;i<3;i++){
    int centerX=(i*43)+21;
    display.setFont(NULL); String d=fcast[i].dayName; if(d=="") d="...";
    display.setCursor(centerX-(d.length()*3),20); display.print(d);
    display.drawBitmap(centerX-8,28,getMiniIcon(fcast[i].iconType),16,16,SSD1306_WHITE);
    int temp = fcast[i].temp;
    display.setFont(&FreeSansBold9pt7b); 
    int16_t x1,y1; uint16_t w,h; String tStr = String(temp);
    display.getTextBounds(tStr, 0, 0, &x1, &y1, &w, &h);
    int tx = centerX - (w / 2) - 2; 
    display.setCursor(tx, 60); display.print(temp); 
    display.fillCircle(tx + w + 3, 52, 2, SSD1306_WHITE); 
  }
}

void drawClock() {
  struct tm t;
  if(!getLocalTime(&t)){ display.setFont(NULL); display.setCursor(30,30); display.print("No Time"); return; }
  String ampm = (t.tm_hour>=12)?"PM":"AM";
  int h12 = t.tm_hour%12; if(h12==0) h12=12;
  
  display.setTextColor(SSD1306_WHITE); display.setFont(NULL);
  display.setCursor(114,0); display.print(ampm);
  
  char timeStr[6]; sprintf(timeStr,"%02d:%02d",h12,t.tm_min);
  char secStr[3]; sprintf(secStr,"%02d", t.tm_sec);

  display.setFont(&FreeSansBold18pt7b);
  int16_t x1,y1; uint16_t w,h;
  display.getTextBounds(timeStr,0,0,&x1,&y1,&w,&h);
  int mainTimeX = (SCREEN_WIDTH - w) / 2 - 10;
  display.setCursor(mainTimeX, 42); display.print(timeStr);

  display.setFont(NULL); display.setCursor(mainTimeX + w + 4, 34); display.print(secStr);

  display.setFont(&FreeSans9pt7b);
  char dateStr[20]; strftime(dateStr,20,"%a, %b %d",&t);
  display.getTextBounds(dateStr,0,0,&x1,&y1,&w,&h);
  display.setCursor((SCREEN_WIDTH-w)/2,62); display.print(dateStr);
}

void drawWeatherCard() {
  display.drawBitmap(96, 0, getBigIcon(weatherMain), 32, 32, SSD1306_WHITE);
  display.setFont(&FreeSansBold9pt7b); String c = city; c.toUpperCase();
  display.setCursor(0, 14); display.print(c);
  display.setFont(&FreeSansBold18pt7b); int tempInt = (int)temperature; 
  display.setCursor(0, 48);
  if (tempInt == 0 && weatherMain == "Offline") display.print("--"); else display.print(tempInt);
  display.fillCircle(40, 26, 4, SSD1306_WHITE);
  display.setFont(NULL); display.drawBitmap(88, 32, bmp_tiny_drop, 8, 8, SSD1306_WHITE);
  display.setCursor(100, 32); display.print(humidity); display.print("%");
  display.setCursor(0, 55); display.print(weatherDesc); 
}

void drawWorldClock() {
  time_t now; time(&now);
  time_t t1 = now + zone1Offset; 
  struct tm tm1_copy; struct tm* temp1 = gmtime(&t1); memcpy(&tm1_copy, temp1, sizeof(struct tm));

  time_t t2 = now + zone2Offset;
  struct tm tm2_copy; struct tm* temp2 = gmtime(&t2); memcpy(&tm2_copy, temp2, sizeof(struct tm));

  display.fillRect(0,0,128,16,SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK); display.setFont(NULL);
  
  int16_t x1,y1; uint16_t w,h;
  display.getTextBounds("WORLD CLOCK", 0, 0, &x1,&y1,&w,&h);
  display.setCursor((128-w)/2, 4); display.print("WORLD CLOCK");
  
  display.setTextColor(SSD1306_WHITE); display.drawLine(64,18,64,54,SSD1306_WHITE);
  
  String n1 = zone1Name.length() > 3 ? zone1Name.substring(0, 3) : zone1Name;
  String n2 = zone2Name.length() > 3 ? zone2Name.substring(0, 3) : zone2Name;
  n1.toUpperCase(); n2.toUpperCase();

  display.setFont(NULL); 
  display.setCursor(12, 22); display.print(n1);
  display.setFont(&FreeSansBold9pt7b); 
  display.setCursor(2,46); display.printf("%02d:%02d", tm1_copy.tm_hour, tm1_copy.tm_min);
  display.setFont(NULL);
  display.setCursor(15, 50); display.printf("%02d/%02d", tm1_copy.tm_mday, tm1_copy.tm_mon+1);

  display.setFont(NULL); 
  display.setCursor(76, 22); display.print(n2);
  display.setFont(&FreeSansBold9pt7b); 
  display.setCursor(66,46); display.printf("%02d:%02d", tm2_copy.tm_hour, tm2_copy.tm_min);
  display.setFont(NULL);
  display.setCursor(79, 50); display.printf("%02d/%02d", tm2_copy.tm_mday, tm2_copy.tm_mon+1);
}

void drawAlarmView() {
    display.clearDisplay();
    display.drawRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE); 
    display.fillRect(0,0,128,16,SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK); display.setFont(NULL);
    display.setCursor(45,4); display.print("ALARM");
    
    display.setTextColor(SSD1306_WHITE);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(20, 50);
    display.printf("%02d:%02d", alarmHour, alarmMin);
    
    display.setFont(NULL);
    display.setCursor(105, 30);
    if(alarmOn) display.print("ON"); else display.print("OFF");
}

void drawAlarmSet() {
    if (millis() - lastBlinkTime > 500) { blinkState = !blinkState; lastBlinkTime = millis(); }
    
    display.clearDisplay();
    display.drawRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE); 
    display.setTextColor(SSD1306_WHITE);
    display.setFont(NULL); display.setCursor(35, 4); display.print("SET ALARM");

    if(editField == 0 && !blinkState) display.setTextColor(SSD1306_BLACK); 
    else display.setTextColor(SSD1306_WHITE);
    display.setFont(&FreeSansBold18pt7b); display.setCursor(10, 50); display.printf("%02d", alarmHour);

    display.setTextColor(SSD1306_WHITE); display.print(":");

    if(editField == 1 && !blinkState) display.setTextColor(SSD1306_BLACK); 
    else display.setTextColor(SSD1306_WHITE);
    display.printf("%02d", alarmMin);

    display.setFont(NULL);
    if(editField == 3 && !blinkState) display.setTextColor(SSD1306_BLACK); 
    else display.setTextColor(SSD1306_WHITE);
    display.setCursor(100, 30); if(alarmOn) display.print("ON"); else display.print("OFF");
}

void drawCountdown() {
    unsigned long elapsed = millis() - countdownStart;
    unsigned long remaining = countdownDuration - elapsed;
    if (elapsed >= countdownDuration) { isAlarmRinging = true; isCountingDown = false; return; }
    
    display.clearDisplay(); 
    display.fillScreen(SSD1306_BLACK); 
    display.drawRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE); 
    display.fillRect(0,0,128,16, SSD1306_WHITE);
    
    display.setTextColor(SSD1306_BLACK); display.setFont(NULL);
    display.setCursor(45, 4); display.print("TIMER");
    
    display.setTextColor(SSD1306_WHITE); display.setFont(&FreeSansBold18pt7b);
    int secRem = (remaining / 1000) % 60; int minRem = (remaining / 60000);
    display.setCursor(20, 50); display.printf("%02d:%02d", minRem, secRem);
    display.display();
}

void drawAlarmRing() {
    display.clearDisplay(); 
    display.fillScreen(SSD1306_BLACK); 
    display.drawRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE); 
    
    display.setTextColor(SSD1306_WHITE); 
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(15, 40); display.print("TIME UP!");
    display.display();
    playAlarmSound();
}

// ==================================================
// INPUT
// ==================================================
void handleTouch() {
  bool currentPinState = digitalRead(TOUCH_PIN);
  unsigned long now = millis();

  if (currentPinState && (isAlarmRinging || alarmTriggered)) {
      isAlarmRinging = false; alarmTriggered = false; isCountingDown = false;
      beepClick(); return; 
  }
  if (currentPinState) lastInteraction = now;

  if (currentPinState && !lastPinState) { pressStartTime = now; isLongPressHandled = false; }
  else if (currentPinState && lastPinState) {
    if ((now - pressStartTime > LONG_PRESS_TIME) && !isLongPressHandled) {
      beepClick();
      
      if (currentPage == 6) { currentPage = 2; } 
      else if (isCountingDown) { isCountingDown = false; currentPage = 0; beep(1000, 200); }
      else if (currentPage == 0) { currentMood++; if (currentMood > MOOD_SUSPICIOUS) currentMood = 0; playMoodSound(currentMood); }
      else if (currentPage == 1) { currentPage = 3; } 
      else if (currentPage == 2) { currentPage = 4; } 
      else if (currentPage == 3) { currentPage = 5; } 
      else if (currentPage == 5) { currentPage = 6; editField = 0; } 
      else if (currentPage == 4) { currentPage = 2; } 
      
      lastPageSwitch = now;
      isLongPressHandled = true;
    }
  }
  else if (!currentPinState && lastPinState) {
    if ((now - pressStartTime < LONG_PRESS_TIME) && !isLongPressHandled) { tapCounter++; lastTapTime = now; }
  }
  lastPinState = currentPinState;

  if (tapCounter > 0 && now - lastTapTime > DOUBLE_TAP_DELAY) {
    if (!isCountingDown) {
        lastPageSwitch = now; beepClick(); 
        
        if (currentPage == 6) { 
            if (tapCounter == 1) { 
                if(editField == 0) { alarmHour++; if(alarmHour > 23) alarmHour = 0; }
                if(editField == 1) { alarmMin++; if(alarmMin > 59) alarmMin = 0; }
                if(editField == 3) { alarmOn = !alarmOn; }
            }
            if (tapCounter == 2) { 
                editField++; if(editField == 2) editField++; 
                if(editField > 3) editField = 0;
            }
        }
        else if (currentPage == 5 && tapCounter == 1) { currentPage = 2; } 
        else if (currentPage == 4 && tapCounter == 1) { currentPage = 2; }
        else {
            if (tapCounter == 3) { isCountingDown = true; countdownDuration = timerMinutes * 60000; countdownStart = millis(); }
            else if (tapCounter == 2) { 
                if(currentPage == 0) {
                    isLowPowerMode = !isLowPowerMode;
                    if(isLowPowerMode) {
                        display.ssd1306_command(SSD1306_SETCONTRAST); display.ssd1306_command(1);
                        currentMood = MOOD_SLEEPY;
                    } else {
                        display.ssd1306_command(SSD1306_SETCONTRAST); display.ssd1306_command(255);
                        currentMood = MOOD_NORMAL;
                    }
                } else {
                    highBrightness = !highBrightness;
                    display.ssd1306_command(SSD1306_SETCONTRAST); display.ssd1306_command(highBrightness ? 255 : 1);
                }
            }
            else if (tapCounter == 1) {
                if (currentPage == 0) currentPage = 1; else if (currentPage == 1) currentPage = 2; else currentPage = 0; 
            }
        }
    }
    tapCounter = 0;
  }
}

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  WiFi.mode(WIFI_STA); WiFi.disconnect(true);

  prefs.begin("bobo", false);
  wifiSSID = prefs.getString("ssid", ""); wifiPASS = prefs.getString("pass", "");
  city = prefs.getString("city", ""); apiKey = prefs.getString("api", "");
  zone1Name = prefs.getString("z1n", "UTC"); zone1Offset = prefs.getLong("z1o", 0);
  zone2Name = prefs.getString("z2n", "Japan"); zone2Offset = prefs.getLong("z2o", 32400);

  storedEyeW = prefs.getFloat("eW", 36); storedEyeH = prefs.getFloat("eH", 36);
  storedEyeR = prefs.getInt("eR", 8); storedEyeX = prefs.getFloat("eX", 0); storedEyeY = prefs.getFloat("eY", 0);
  soundEnabled = prefs.getBool("snd", true); chimeEnabled = prefs.getBool("chm", true);
  waterEnabled = prefs.getBool("wtr", false); timerMinutes = prefs.getInt("tmr", 10);
  
  alarmHour = prefs.getInt("aH", 7); alarmMin = prefs.getInt("aM", 0); alarmOn = prefs.getBool("aOn", false);

  bootLine1 = prefs.getString("bt1", "Booting"); bootLine2 = prefs.getString("bt2", "BOBO 2.2");
  bootLine3 = prefs.getString("bt3", "Loading..."); bootLine4 = prefs.getString("bt4", "Here We Go");

  if (wifiSSID.length() > 0) wifiEnabled = true;

  Wire.begin(SDA_PIN, SCL_PIN); 
  Wire.setClock(400000); 

  pinMode(TOUCH_PIN, INPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);


  runBootSequence(&display, bootLine1, bootLine2, bootLine3, bootLine4); 
  playStartupSound();
  leftEye.init(18 + storedEyeX, 14 + storedEyeY, storedEyeW, storedEyeH);
  rightEye.init(74 + storedEyeX, 14 + storedEyeY, storedEyeW, storedEyeH);

  lastWeatherUpdate = millis(); lastPageSwitch = millis();
  lastInteraction = millis(); lastWaterAlert = millis();
}

// ==================================================
// MAIN LOOP
// ==================================================
void loop() {
  unsigned long now = millis();

  // 1. HIGH PRIORITY: DRAWING & PHYSICS
  if (isAlarmRinging || alarmTriggered) { drawAlarmRing(); handleTouch(); return; }
  if (isCountingDown) { drawCountdown(); handleTouch(); return; }

  display.clearDisplay();
  
  // PAGE ROUTER
  switch(currentPage){
    case 0: drawEmoPage(); break;
    case 1: drawClock(); break;
    case 2: drawWeatherCard(); break;
    case 3: drawWorldClock(); break;
    case 4: drawForecastPage(); break;
    case 5: drawAlarmView(); break;
    case 6: drawAlarmSet(); break;
  }
  
  display.display();
  handleTouch(); 

  // 2. BACKGROUND TASKS (Throttled)
  if (now - lastHeavyTaskTime > 3000) {
    checkConnections();
    
    while(Serial.available()){
      char c = Serial.read();
      if(c == '\n'){ handleSerialCommand(serialBuffer); serialBuffer = ""; }
      else if(c != '\r'){ serialBuffer += c; }
    }

    if(wifiEnabled && WiFi.status() == WL_CONNECTED && (now - lastWeatherUpdate > weatherInterval)) {
       getWeatherAndForecast(); lastWeatherUpdate = now; 
    }

    struct tm t;
    if (getLocalTime(&t)) {
        static int lastHour = -1; 
        if (chimeEnabled && t.tm_min == 0 && t.tm_hour != lastHour) {
            lastHour = t.tm_hour; beep(2000, 100); delay(100); beep(2000, 100);
        }
        if (t.tm_min != 0) lastHour = -1; 
        
        if (alarmOn && t.tm_hour == alarmHour && t.tm_min == alarmMin && t.tm_sec < 5) {
            if(!alarmTriggered) { alarmTriggered = true; }
        }
    }

    if (waterEnabled && (now - lastWaterAlert > WATER_INTERVAL)) {
        beepAlert(); lastWaterAlert = now;
        display.clearDisplay(); 
        display.drawRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE); 
        display.setFont(&FreeSansBold18pt7b);
        display.setCursor(20, 28); display.print("Drink");
        display.setCursor(10, 60); display.print("Water!");
        display.display(); delay(2000); 
    }

    if (currentPage < 3 && (now - lastPageSwitch > currentPageInterval)) {
      currentPage++; if (currentPage > 2) currentPage = 0;
      lastPageSwitch = now; lastSaccade = 0; 
    }
    
    lastHeavyTaskTime = now;
  }
}
