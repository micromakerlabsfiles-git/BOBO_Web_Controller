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
#include "boot_sequence_spamworld.h"

Preferences prefs;

// ==================================================
// 1. ASSETS & CONFIG
// ==================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 20
#define SCL_PIN 21
#define TOUCH_PIN 1

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool timeConfigured = false;

// ==================================================
// WEB SERIAL CONTROL & STORAGE
// ==================================================
String serialBuffer = "";
bool webOverrideEyes = false;
unsigned long webEyeTimeout = 0;

// PERSISTENT EYE SETTINGS (New Variables)
float storedEyeW = 36;
float storedEyeH = 36;
int storedEyeR = 8;    // Radius
float storedEyeX = 0;  // Offset X
float storedEyeY = 0;  // Offset Y

// --- TIMING VARIABLES ---
unsigned long lastWeatherUpdate = 0;
unsigned long weatherInterval = 7200000; // 2 HOURS (Fixed per request)

unsigned long lastPageSwitch = 0;
unsigned long currentPageInterval = 25000; // Starts with Eyes duration

// CONSTANTS FOR DURATIONS
const long DURATION_EYES = 25000; // 25 Seconds for Eyes
const long DURATION_INFO = 5000;  // 5 Seconds for Clock/Weather (Change this if needed)

// ==================================================
// WIFI (RUNTIME, NON-BLOCKING)
// ==================================================
bool wifiEnabled = false;
String wifiSSID = "";
String wifiPASS = "";
unsigned long lastWifiAttempt = 0;

// ==================================================
// WEATHER & TIME
// ==================================================
String city = "";
const char* countryCode = "IN";
String apiKey = "";
float temperature = 0.0;
float feelsLike = 0.0;
int humidity = 0;
String weatherMain = "Offline";
String weatherDesc = "No WiFi";

// WORLD CLOCK GLOBALS
long zone1Offset = 0;      // Offset in seconds
String zone1Name = "UTC";
long zone2Offset = 32400;  // Default Tokyo (9*3600)
String zone2Name = "JAPAN";

struct ForecastDay {
  String dayName;
  int temp;
  String iconType;
};
ForecastDay fcast[3];

const char* ntpServer = "pool.ntp.org";
const char* tzString = "IST-5:30";

// --- WEATHER ICONS ---
const unsigned char bmp_clear[] PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0xc0,0x80,0x00,0x0f,0xf0,0x00,0x00,0x3f,0xfc,0x00,0x00,0x7f,0xfe,0x00,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x00,0x7f,0xfe,0x00,0x00,0x3f,0xfc,0x00,0x01,0x0f,0xf0,0x80,0x00,0x03,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_clouds[] PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x3f,0xff,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x00,0xff,0xff,0xe0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_rain[] PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x0c,0x00,0x00,0x60,0x0c,0x00,0x00,0xe0,0x1c,0x00,0x00,0xc0,0x18,0x00,0x03,0x80,0x70,0x00,0x03,0x80,0x70,0x00,0x03,0x00,0x60,0x00,0x02,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char mini_sun[] PROGMEM = { 0x00,0x00,0x01,0x80,0x00,0x00,0x10,0x08,0x04,0x20,0x03,0xc0,0x27,0xe4,0x07,0xe0,0x07,0xe0,0x27,0xe4,0x03,0xc0,0x04,0x20,0x10,0x08,0x00,0x00,0x01,0x80,0x00,0x00 };
const unsigned char mini_cloud[] PROGMEM = { 0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x0f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char mini_rain[] PROGMEM = { 0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x00,0x00,0x44,0x44,0x22,0x22,0x11,0x11 };
const unsigned char bmp_tiny_drop[] PROGMEM = { 0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x7c, 0x38, 0x00 };

// ==================================================
// UI STATE
// ==================================================
int currentPage = 0;
bool highBrightness = true;

int tapCounter = 0;
unsigned long lastTapTime = 0;
bool lastPinState = false;

unsigned long pressStartTime = 0;
bool isLongPressHandled = false;
const unsigned long LONG_PRESS_TIME = 800;
const unsigned long DOUBLE_TAP_DELAY = 300;
const unsigned long PAGE_INTERVAL = 8000;

// ==================================================
// MOODS
// ==================================================
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
void getWeatherAndForecast();

// ==================================================
// 2. ULTRA PRO PHYSICS ENGINE
// ==================================================
struct Eye {
  float x, y;
  float w, h;
  float targetX, targetY, targetW, targetH;

  float pupilX, pupilY;
  float targetPupilX, targetPupilY;

  float velX, velY, velW, velH;
  float pVelX, pVelY;
  float k = 0.12;
  float d = 0.60;
  float pk = 0.08;
  float pd = 0.50;

  bool blinking;
  unsigned long lastBlink;
  unsigned long nextBlinkTime;

  void init(float _x, float _y, float _w, float _h) {
    x = targetX = _x;
    y = targetY = _y;
    w = targetW = _w;
    h = targetH = _h;
    pupilX = targetPupilX = 0;
    pupilY = targetPupilY = 0;
    nextBlinkTime = millis() + random(1000, 4000);
  }

  void update() {
    float ax = (targetX - x) * k;
    float ay = (targetY - y) * k;
    float aw = (targetW - w) * k;
    float ah = (targetH - h) * k;

    velX = (velX + ax) * d;
    velY = (velY + ay) * d;
    velW = (velW + aw) * d;
    velH = (velH + ah) * d;

    x += velX;
    y += velY;
    w += velW;
    h += velH;

    float pax = (targetPupilX - pupilX) * pk;
    float pay = (targetPupilY - pupilY) * pk;
    pVelX = (pVelX + pax) * pd;
    pVelY = (pVelY + pay) * pd;

    pupilX += pVelX;
    pupilY += pVelY;
  }
};

Eye leftEye, rightEye;
unsigned long lastSaccade = 0;
unsigned long saccadeInterval = 3000;
float breathVal = 0;

// ==================================================
// WEATHER ICON HELPERS
// ==================================================
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

// ==================================================
// 3. FORECAST FETCH (NEW)
// ==================================================
void getForecast() {
  if (!wifiEnabled || WiFi.status() != WL_CONNECTED || apiKey == "") return;
  
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/forecast?q=" +
               city + "," + countryCode +
               "&appid=" + apiKey + "&units=metric&cnt=25";
               
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    JSONVar o = JSON.parse(http.getString());
    
    // Pick 3 points approx 24h apart (Indices 7, 15, 23)
    int indices[3] = {7, 15, 23}; 
    
    for (int i = 0; i < 3; i++) {
      int idx = indices[i];
      fcast[i].temp = int(o["list"][idx]["main"]["temp"]);
      String raw = (const char*)o["list"][idx]["weather"][0]["main"];
      fcast[i].iconType = raw;

      long dt = (long)o["list"][idx]["dt"];
      time_t t = dt;
      struct tm* timeInfo = localtime(&t);
      char dayBuf[4];
      strftime(dayBuf, 4, "%a", timeInfo); 
      fcast[i].dayName = String(dayBuf);
    }
    Serial.println("Forecast Updated");
  } 
  http.end();
}

// ==================================================
// SERIAL COMMAND HANDLER
// ==================================================
void handleSerialCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  int sep = cmd.indexOf(':');
  String key, val;

  if (sep >= 0) {
    key = cmd.substring(0, sep);
    val = cmd.substring(sep + 1);
  } else {
    key = cmd;
    val = "";
  }

  key.toUpperCase();

  // ================= SYSTEM =================
  if (key == "REBOOT") {
    Serial.println("OK REBOOTING...");
    delay(200);
    ESP.restart();
  }

  // ================= WIFI =================
  else if (key == "WIFI_SSID") {
    wifiSSID = val;
    wifiEnabled = true; 
    Serial.println("OK WIFI_SSID");
  }
  else if (key == "WIFI_PASS") {
    wifiPASS = val;
    Serial.println("OK WIFI_PASS");
  }
  else if (key == "WIFI_ON") {
    wifiEnabled = true;
    lastWifiAttempt = 0;
    Serial.println("OK WIFI_ON");
  }
  else if (key == "WIFI_OFF") {
    wifiEnabled = false;
    WiFi.disconnect(true);
    timeConfigured = false;
    Serial.println("OK WIFI_OFF");
  }

  // ================= WEATHER =================
  else if (key == "CITY") {
    city = val;
    Serial.println("OK CITY");
    getWeatherAndForecast();
  }
  else if (key == "APIKEY") {
    apiKey = val;
    Serial.println("OK APIKEY");
    getWeatherAndForecast();
  }

  // ================= UI =================
  else if (key == "MOOD") {
    int m = val.toInt();
    if (m >= 0 && m <= MOOD_SUSPICIOUS) {
      currentMood = m;
      lastSaccade = 0;
      Serial.println("OK MOOD");
    }
  }
  else if (key == "PAGE") {
    int p = val.toInt();
    if (p >= 0 && p <= 4) {
      currentPage = p;
      lastPageSwitch = millis();
      Serial.println("OK PAGE");
    }
  }
  else if (key == "BRIGHT") {
    int b = constrain(val.toInt(), 1, 255);
    Wire.beginTransmission(0x3C);
    Wire.write(0x00);
    Wire.write(0x81);
    Wire.write(b);
    Wire.endTransmission();
    highBrightness = (b > 100);
    Serial.println("OK BRIGHT");
  }

  // ================= EYES (FIXED) =================
  else if (key == "EYE") {
    int c = val.indexOf(',');
    if (c > 0) {
      float x = constrain(val.substring(0, c).toFloat(), -10, 10);
      float y = constrain(val.substring(c + 1).toFloat(), -10, 10);

      leftEye.targetPupilX = x;
      leftEye.targetPupilY = y;
      rightEye.targetPupilX = x;
      rightEye.targetPupilY = y;
      webOverrideEyes = true;
      webEyeTimeout = millis();
      Serial.println("OK EYE");
    }
  }
  else if (key == "EYE_POS") {
    int c = val.indexOf(',');
    if (c > 0) {
      storedEyeX = val.substring(0, c).toFloat();
      storedEyeY = val.substring(c + 1).toFloat();

      leftEye.targetX = 18 + storedEyeX;
      leftEye.targetY = 14 + storedEyeY;
      rightEye.targetX = 74 + storedEyeX;
      rightEye.targetY = 14 + storedEyeY;

      Serial.println("OK EYE_POS");
    }
  }
  else if (key == "EYE_SIZE") {
    int c = val.indexOf(',');
    if (c > 0) {
      storedEyeW = constrain(val.substring(0, c).toFloat(), 10, 60);
      storedEyeH = constrain(val.substring(c + 1).toFloat(), 10, 60);
      Serial.println("OK EYE_SIZE");
    }
  }
  else if (key == "EYE_RADIUS") {
    storedEyeR = constrain(val.toInt(), 0, 20);
    Serial.println("OK EYE_RADIUS");
  }
  else if (key == "EYE_SAVE") {
    prefs.putFloat("eW", storedEyeW);
    prefs.putFloat("eH", storedEyeH);
    prefs.putInt("eR", storedEyeR);
    prefs.putFloat("eX", storedEyeX);
    prefs.putFloat("eY", storedEyeY);
    Serial.println("OK EYES SAVED");
  }
  else if (key == "BLINK") {
    leftEye.blinking = true;
    rightEye.blinking = true;
    leftEye.lastBlink = millis();
    Serial.println("OK BLINK");
  }

  // ================= WORLD CLOCK =================
  else if (key == "Z1_SET") {
    int c = val.indexOf(',');
    if (c > 0) {
      zone1Name = val.substring(0, c);
      zone1Offset = val.substring(c + 1).toInt();
      Serial.println("OK Z1");
    }
  }
  else if (key == "Z2_SET") {
    int c = val.indexOf(',');
    if (c > 0) {
      zone2Name = val.substring(0, c);
      zone2Offset = val.substring(c + 1).toInt();
      Serial.println("OK Z2");
    }
  }

  // ================= SAVE ALL =================
  else if (key == "SAVE") {
    prefs.putString("ssid", wifiSSID);
    prefs.putString("pass", wifiPASS);
    prefs.putString("city", city);
    prefs.putString("api", apiKey);
    
    prefs.putString("z1n", zone1Name);
    prefs.putLong("z1o", zone1Offset);
    prefs.putString("z2n", zone2Name);
    prefs.putLong("z2o", zone2Offset);
    
    if(wifiEnabled) { WiFi.disconnect(); lastWifiAttempt = 0; }
    Serial.println("OK SAVED");
  }
}


// ==================================================
// WIFI HANDLER
// ==================================================
void handleWifi() {
  if (!wifiEnabled) return;
  if (WiFi.status() == WL_CONNECTED) {
    if (!timeConfigured) {
      configTime(0, 0, ntpServer);
      setenv("TZ", tzString, 1);
      tzset();
      timeConfigured = true;
      Serial.println("NTP started");
      getWeatherAndForecast();
    }
    return;
  }

  if (wifiSSID.length() == 0) return;
  if (millis() - lastWifiAttempt < 10000) return;

  lastWifiAttempt = millis();
  Serial.println("WiFi: Connecting...");
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
}


// ==================================================
// WEATHER UPDATE
// ==================================================
void getWeatherAndForecast() {
  if (!wifiEnabled || WiFi.status() != WL_CONNECTED || apiKey == "") return;
  
  // 1. Current Weather
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
               city + "," + countryCode +
               "&appid=" + apiKey + "&units=metric";
  http.begin(url);
  if (http.GET() == 200) {
    JSONVar o = JSON.parse(http.getString());
    temperature = double(o["main"]["temp"]);
    feelsLike = double(o["main"]["feels_like"]);
    humidity = int(o["main"]["humidity"]);
    weatherMain = (const char*)o["weather"][0]["main"];
    weatherDesc = (const char*)o["weather"][0]["description"];
    weatherDesc[0] = toupper(weatherDesc[0]);
  }
  http.end();

  // 2. Forecast
  getForecast();
}


// ==================================================
// MOOD LOGIC
// ==================================================
void updateMoodBasedOnWeather() {
  int m = MOOD_NORMAL;
  if (weatherMain == "Clear") m = MOOD_HAPPY;
  else if (weatherMain == "Rain" || weatherMain == "Drizzle") m = MOOD_SAD;
  else if (weatherMain == "Thunderstorm") m = MOOD_SURPRISED;
  else if (weatherMain == "Clouds") m = MOOD_NORMAL;
  else if (temperature > 25) m = MOOD_EXCITED;
  else if (temperature < 15) m = MOOD_SLEEPY;
  currentMood = m;
}

// ==================================================
// TOUCH INPUT
// ==================================================
void handleTouch() {
  bool currentPinState = digitalRead(TOUCH_PIN);
  unsigned long now = millis();
  if (currentPinState && !lastPinState) {
    pressStartTime = now;
    isLongPressHandled = false;
  }
  else if (currentPinState && lastPinState) {
    if ((now - pressStartTime > LONG_PRESS_TIME) && !isLongPressHandled) {
      lastPageSwitch = now;
      if (currentPage == 0) {
        currentMood++;
        if (currentMood > MOOD_SUSPICIOUS) currentMood = 0;
        lastSaccade = 0;
      }
      else if (currentPage == 1) currentPage = 3;
      else if (currentPage == 2) currentPage = 4;
      isLongPressHandled = true;
    }
  }
  else if (!currentPinState && lastPinState) {
    if ((now - pressStartTime < LONG_PRESS_TIME) && !isLongPressHandled) {
      tapCounter++;
      lastTapTime = now;
    }
  }

  lastPinState = currentPinState;
  if (tapCounter > 0 && now - lastTapTime > DOUBLE_TAP_DELAY) {
    lastPageSwitch = now;
    if (tapCounter == 2) {
      highBrightness = !highBrightness;
      uint8_t b = highBrightness ? 255 : 1;
      Wire.beginTransmission(0x3C);
      Wire.write(0x00);
      Wire.write(0x81);
      Wire.write(b);
      Wire.endTransmission();
    }
    else if (tapCounter == 1) {
      if (currentPage == 3) currentPage = 1;
      else if (currentPage == 4) currentPage = 2;
      else {
        currentPage++;
        if (currentPage > 2) currentPage = 0;
      }
    }
    tapCounter = 0;
  }
}

// ==================================================
// EYELID MASKS
// ==================================================
void drawEyelidMask(float x, float y, float w, float h, int mood, bool isLeft) {
  int ix = (int)x;
  int iy = (int)y;
  int iw = (int)w;
  int ih = (int)h;
  if (mood == MOOD_ANGRY) {
    if (isLeft)
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy + i, ix + iw, iy - 6 + i, SSD1306_BLACK);
    else
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy - 6 + i, ix + iw, iy + i, SSD1306_BLACK);
  }
  else if (mood == MOOD_SAD) {
    if (isLeft)
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy - 6 + i, ix + iw, iy + i, SSD1306_BLACK);
    else
      for (int i = 0; i < 16; i++) display.drawLine(ix, iy + i, ix + iw, iy - 6 + i, SSD1306_BLACK);
  }
  else if (mood == MOOD_HAPPY || mood == MOOD_LOVE || mood == MOOD_EXCITED) {
    display.fillRect(ix, iy + ih - 12, iw, 14, SSD1306_BLACK);
    display.fillCircle(ix + iw / 2, iy + ih + 6, iw / 1.3, SSD1306_BLACK);
  }
  else if (mood == MOOD_SLEEPY) {
    display.fillRect(ix, iy, iw, ih / 2 + 2, SSD1306_BLACK);
  }
  else if (mood == MOOD_SUSPICIOUS) {
    if (isLeft) display.fillRect(ix, iy, iw, ih / 2 - 2, SSD1306_BLACK);
    else display.fillRect(ix, iy + ih - 8, iw, 8, SSD1306_BLACK);
  }
}

// ==================================================
// DRAW ULTRA PRO EYE (FIXED)
// ==================================================
void drawUltraProEye(Eye &e, bool isLeft) {
  int ix = (int)e.x;
  int iy = (int)e.y;
  int iw = (int)e.w;
  int ih = (int)e.h;

  int r = storedEyeR; // Use stored radius
  if (r > iw/2) r = iw/2;
  if (r > ih/2) r = ih/2;
  if (r < 0) r = 0;

  display.fillRoundRect(ix, iy, iw, ih, r, SSD1306_WHITE);
  int cx = ix + iw / 2;
  int cy = iy + ih / 2;
  int pw = iw / 2.2;
  int ph = ih / 2.2;
  int px = cx + (int)e.pupilX - pw / 2;
  int py = cy + (int)e.pupilY - ph / 2;
  px = constrain(px, ix, ix + iw - pw);
  py = constrain(py, iy, iy + ih - ph);
  display.fillRoundRect(px, py, pw, ph, r / 2, SSD1306_BLACK);

  if (iw > 15 && ih > 15) {
    display.fillCircle(px + pw - 4, py + 4, 2, SSD1306_WHITE);
  }

  drawEyelidMask(e.x, e.y, e.w, e.h, currentMood, isLeft);
}

// ==================================================
// PHYSICS & MOOD (FIXED)
// ==================================================
void updatePhysicsAndMood() {
  unsigned long now = millis();
  breathVal = sin(now / 800.0) * 1.5;

  if (now > leftEye.nextBlinkTime) {
    leftEye.blinking = true;
    rightEye.blinking = true;
    leftEye.lastBlink = now;
    leftEye.nextBlinkTime = now + random(2000, 6000);
  }

  if (leftEye.blinking) {
    leftEye.targetH = 2;
    rightEye.targetH = 2;
    if (now - leftEye.lastBlink > 120) {
      leftEye.blinking = false;
      rightEye.blinking = false;
    }
  }

  if (!leftEye.blinking && !webOverrideEyes &&
      now - lastSaccade > saccadeInterval) {
    lastSaccade = now;
    saccadeInterval = random(500, 3000);
    float lx = random(-8, 9);
    float ly = random(-4, 5);

    leftEye.targetPupilX = lx;
    leftEye.targetPupilY = ly;
    rightEye.targetPupilX = lx;
    rightEye.targetPupilY = ly;

    // Use stored offsets
    leftEye.targetX = 18 + storedEyeX + lx * 0.3;
    leftEye.targetY = 14 + storedEyeY + ly * 0.3;
    rightEye.targetX = 74 + storedEyeX + lx * 0.3;
    rightEye.targetY = 14 + storedEyeY + ly * 0.3;
  }

  if (!leftEye.blinking) {
    // START with stored user settings
    float baseW = storedEyeW;
    float baseH = storedEyeH + breathVal; 

    switch (currentMood) {
      case MOOD_HAPPY:
      case MOOD_LOVE:
        baseW = 40; baseH = 32; break;
      case MOOD_SURPRISED:
        baseW = 30; baseH = 45; break;
      case MOOD_SLEEPY:
        baseW = 38; baseH = 30; break;
      case MOOD_ANGRY:
        baseW = 34; baseH = 32; break;
      case MOOD_SAD:
        baseW = 34; baseH = 40; break;
      case MOOD_SUSPICIOUS:
        leftEye.targetH = 20;
        rightEye.targetH = 42;
        break;
    }

    leftEye.targetW = baseW;
    leftEye.targetH = baseH;
    rightEye.targetW = baseW;
    rightEye.targetH = baseH;
    
    // Suspicious override must happen after assignment
    if(currentMood == MOOD_SUSPICIOUS) {
        leftEye.targetH = 20;
        rightEye.targetH = 42;
    }
  }

  if (webOverrideEyes) {
    // If user is manually moving eyes, we don't update targets here 
    // (handled by handleSerialCommand)
    // but we can add breath effect if desired, or keep it static
  }

  leftEye.update();
  rightEye.update();

  if (webOverrideEyes && millis() - webEyeTimeout > 2000) {
    webOverrideEyes = false;
  }
}

// ==================================================
// PAGES
// ==================================================
void drawEmoPage() {
  updatePhysicsAndMood();
  drawUltraProEye(leftEye, true);
  drawUltraProEye(rightEye, false);
}

void drawForecastPage() {
  display.fillRect(0,0,128,16,SSD1306_WHITE);
  display.setFont(NULL);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(20,4);
  display.print("3-DAY FORECAST");

  display.setTextColor(SSD1306_WHITE);
  display.drawLine(42,16,42,64,SSD1306_WHITE);
  display.drawLine(85,16,85,64,SSD1306_WHITE);
  for(int i=0;i<3;i++){
    int xStart=i*43;
    int centerX=xStart+21;

    display.setFont(NULL);
    String d=fcast[i].dayName;
    if(d=="") d="...";
    display.setCursor(centerX-(d.length()*3),20);
    display.print(d);

    display.drawBitmap(centerX-8,28,getMiniIcon(fcast[i].iconType),16,16,SSD1306_WHITE);

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(centerX-6,60);
    display.print(fcast[i].temp);
    display.fillCircle(centerX+8,52,2,SSD1306_WHITE);
  }
}

void drawClock() {
  struct tm t;
  if(!getLocalTime(&t)){
    display.setFont(NULL);
    display.setCursor(30,30);
    display.print("No Time");
    return;
  }

  String ampm = (t.tm_hour>=12)?"PM":"AM";
  int h12 = t.tm_hour%12;
  if(h12==0) h12=12;

  display.setTextColor(SSD1306_WHITE);
  display.setFont(NULL);
  display.setCursor(114,0);
  display.print(ampm);

  display.setFont(&FreeSansBold18pt7b);
  char timeStr[6];
  sprintf(timeStr,"%02d:%02d",h12,t.tm_min);
  int16_t x1,y1; uint16_t w,h;
  display.getTextBounds(timeStr,0,0,&x1,&y1,&w,&h);
  display.setCursor((SCREEN_WIDTH-w)/2,42);
  display.print(timeStr);

  display.setFont(&FreeSans9pt7b);
  char dateStr[20];
  strftime(dateStr,20,"%a, %b %d",&t);
  display.getTextBounds(dateStr,0,0,&x1,&y1,&w,&h);
  display.setCursor((SCREEN_WIDTH-w)/2,62);
  display.print(dateStr);
}

void drawWeatherCard() {
  // 1. Check if we have data (Offline check)
  // We removed the "WiFi.status()" check so it shows cached data
  
  // 2. Icon Logic
  display.drawBitmap(96, 0, getBigIcon(weatherMain), 32, 32, SSD1306_WHITE);

  // 3. City Name
  display.setFont(&FreeSansBold9pt7b);
  String c = city; 
  c.toUpperCase();
  display.setCursor(0, 14);
  display.print(c);

  // 4. Temperature
  display.setFont(&FreeSansBold18pt7b);
  int tempInt = (int)temperature; 
  display.setCursor(0, 48);
  
  // Basic check: if temp is 0 and we are truly offline/empty
  if (tempInt == 0 && weatherMain == "Offline") {
    display.print("--");
  } else {
    display.print(tempInt);
  }
  display.fillCircle(40, 26, 4, SSD1306_WHITE);

  // 5. Humidity
  display.setFont(NULL);
  display.drawBitmap(88, 32, bmp_tiny_drop, 8, 8, SSD1306_WHITE);
  display.setCursor(100, 32);
  display.print(humidity);
  display.print("%");

  // 6. Description
  display.setCursor(0, 55);
  // FIX: Use 'weatherDesc' (your variable) not 'weatherDescription'
  display.print(weatherDesc); 
}

void drawWorldClock() {
  time_t now; time(&now);

  time_t t1 = now + zone1Offset;
  time_t t2 = now + zone2Offset;
  struct tm* tm1 = gmtime(&t1);
  int h1 = tm1->tm_hour; int m1 = tm1->tm_min;
  struct tm* tm2 = gmtime(&t2);
  int h2 = tm2->tm_hour; int m2 = tm2->tm_min;

  display.fillRect(0,0,128,16,SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setFont(NULL);
  display.setCursor(32,4);
  display.print("WORLD CLOCK");

  display.setTextColor(SSD1306_WHITE);
  display.drawLine(64,18,64,54,SSD1306_WHITE);

  // Zone 1
  display.setFont(NULL);
  int w1 = zone1Name.length() * 6; 
  display.setCursor(32 - (w1/2), 22); 
  display.print(zone1Name);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(5,46);
  display.printf("%02d:%02d", h1, m1);

  // Zone 2
  display.setFont(NULL);
  int w2 = zone2Name.length() * 6;
  display.setCursor(96 - (w2/2), 22); 
  display.print(zone2Name);
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(69,46);
  display.printf("%02d:%02d", h2, m2);
}

// ==================================================
// BOOT
// ==================================================
// void playBootAnimation() {
//   int cx=64, cy=32;
//   for(int r=0;r<80;r+=4){
//     display.clearDisplay();
//     display.fillCircle(cx,cy,r,SSD1306_WHITE);
//     display.display();
//     delay(10);
//   }
//   display.clearDisplay();
//   display.setFont(&FreeSansBold9pt7b);
//   display.setCursor(38,36);
//   display.print("BOBO 2.0");
//   display.display();
//   delay(1500);
// }

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  // NVS START
  prefs.begin("bobo", false);
  wifiSSID = prefs.getString("ssid", "");
  wifiPASS = prefs.getString("pass", "");
  city     = prefs.getString("city", "");
  apiKey   = prefs.getString("api", "");
  
  // Load World Clock
  zone1Name = prefs.getString("z1n", "UTC");
  zone1Offset = prefs.getLong("z1o", 0);
  zone2Name = prefs.getString("z2n", "Japan");
  zone2Offset = prefs.getLong("z2o", 32400);

  // Load Eye Settings
  storedEyeW = prefs.getFloat("eW", 36);
  storedEyeH = prefs.getFloat("eH", 36);
  storedEyeR = prefs.getInt("eR", 8);
  storedEyeX = prefs.getFloat("eX", 0);
  storedEyeY = prefs.getFloat("eY", 0);
  
  if (wifiSSID.length() > 0) {
    wifiEnabled = true;
  }

  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(TOUCH_PIN, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // --- NEW BOOT SEQUENCE ---
  runBootSequence(&display); 
  // -------------------------

  leftEye.init(18 + storedEyeX, 14 + storedEyeY, storedEyeW, storedEyeH);
  rightEye.init(74 + storedEyeX, 14 + storedEyeY, storedEyeW, storedEyeH);

  lastWeatherUpdate = millis();
  lastPageSwitch = millis();
}


// ==================================================
// LOOP
// ==================================================
void loop() {
  // -------------------------------------------------
  // 1. HANDLE WEB/SERIAL COMMANDS
  // -------------------------------------------------
  handleWifi(); 
  
  while(Serial.available()){
    char c = Serial.read();
    if(c == '\n'){
      handleSerialCommand(serialBuffer); // Correct Name
      serialBuffer = "";
    } else if(c != '\r'){
      serialBuffer += c;
    }
  }

  // -------------------------------------------------
  // 2. WEATHER UPDATE LOGIC (Every 2 Hours)
  // -------------------------------------------------
  unsigned long now = millis();
  
  // Check if 2 hours (7200000 ms) passed
  if(wifiEnabled && WiFi.status() == WL_CONNECTED && (now - lastWeatherUpdate > weatherInterval)) {
     getWeatherAndForecast(); // Correct Name
     lastWeatherUpdate = now;
     Serial.println("Weather Auto-Update");
  }

  // -------------------------------------------------
  // 3. PAGE SWITCHING LOGIC (Dynamic Timing)
  // -------------------------------------------------
  handleTouch(); 

  // Decide how long to wait based on current page
  if (currentPage == 0) {
    currentPageInterval = DURATION_EYES; // 25 seconds
  } else {
    currentPageInterval = DURATION_INFO; // 5 seconds
  }

  if (currentPage < 3 && (now - lastPageSwitch > currentPageInterval)) {
    currentPage++;
    // Cycle: 0 -> 1 -> 2 -> 0... (Skipping 3/4 unless manually tapped)
    if (currentPage > 2) currentPage = 0; 
    
    lastPageSwitch = now;
    lastSaccade = 0; // Reset eye movement timer
  }

  // -------------------------------------------------
  // 4. DRAWING LOGIC
  // -------------------------------------------------
  display.clearDisplay();
  
  switch(currentPage){
    case 0: 
      drawEmoPage();       // Correct Name (Handles physics + drawing)
      break;
    case 1: 
      drawClock();         // Correct Name
      break;
    case 2: 
      drawWeatherCard();   // Correct Name
      break;
    case 3: 
      drawWorldClock();    // Correct Name
      break;
    case 4: 
      drawForecastPage();  // Correct Name
      break;
  }
  display.display();
}
