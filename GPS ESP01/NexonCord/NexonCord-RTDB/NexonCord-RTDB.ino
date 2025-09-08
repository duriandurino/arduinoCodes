#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ====== Firebase RTDB config ======
#define DATABASE_URL "https://nexon-cord-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define API_KEY "AIzaSyAQYUM31DCBeE7kGts-9_6_m3u4ZRMOeiY"

// ====== Relays ======
#define RELAY1_PIN 23
#define RELAY2_PIN 22
#define RELAY3_PIN 21
#define RELAY4_PIN 19
const int NUM_SWITCHES = 4;
const int relayPins[NUM_SWITCHES] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};

// ====== AP mode ======
const char* AP_SSID = "NexonCord-Config";
const char* AP_PASSWORD = "12345678";

// ====== Globals ======
WebServer server(80);
Preferences preferences;

String wifiSSID;
String wifiPassword;
String fbEmail;
String fbPassword;

FirebaseData fbdo;
FirebaseData stream;
FirebaseAuth auth;
FirebaseConfig config;

bool firebaseInitialized = false;
bool isAPMode = false;
String deviceID;

// ====== Helpers ======
static inline void setRelay(int idx1based, bool on) {
  digitalWrite(relayPins[idx1based - 1], on ? LOW : HIGH); // Active LOW
}

static inline bool relayIsOn(int idx1based) {
  return digitalRead(relayPins[idx1based - 1]) == LOW;
}

String baseSwitchesPath() {
  return "devices/" + deviceID + "/switches";
}

String switchPath(int idx1based) {
  return baseSwitchesPath() + "/switch" + String(idx1based) + "/app_on";
}

int switchIndexFromPath(const String& dataPath) {
  int s = dataPath.indexOf("switch");
  if (s < 0) return -1;
  s += 6;
  int e = s;
  while (e < (int)dataPath.length() && isDigit(dataPath[e])) e++;
  if (e <= s) return -1;
  return dataPath.substring(s, e).toInt();
}

// ====== Forward decls ======
void startAPMode();
void handleRoot();
void handleConfig();
void handleToggle();
void handleScan();
void connectToWiFi();
void initializeFirebase();
void ensureDBStructure();
void syncRelaysFromRTDB();
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting ESP32 with RTDB stream...");

  for (int i = 0; i < NUM_SWITCHES; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // default OFF
  }

  preferences.begin("wifi-config", false);
  wifiSSID   = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("password", "");
  fbEmail    = preferences.getString("fb_email", "");
  fbPassword = preferences.getString("fb_password", "");

  if (wifiSSID.isEmpty() || fbEmail.isEmpty()) {
    Serial.println("No saved credentials. Starting AP mode.");
    startAPMode();
  } else {
    connectToWiFi();
  }
}

// ====== Loop ======
void loop() {
  if (isAPMode) {
    server.handleClient();
    yield();
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, retrying...");
    connectToWiFi();
    return;
  }

  if (firebaseInitialized) {
    if (!Firebase.RTDB.readStream(&stream)) {
      Serial.printf("Stream error: %s\n", stream.errorReason().c_str());
    }

    // Token auto-refresh handled internally, but must call this
    if (!Firebase.ready()) {
      Serial.println("Firebase not ready (token refresh in progress).");
    }
  }
  yield();
}

// ====== WiFi + Firebase ======
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connected to %s, IP: %s\n", wifiSSID.c_str(), WiFi.localIP().toString().c_str());

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    deviceID = "ESP-" + mac;
    Serial.println("Device ID: " + deviceID);

    initializeFirebase();
  } else {
    Serial.println("WiFi connect failed. Starting AP mode.");
    startAPMode();
  }
}

void initializeFirebase() {
  Serial.println("Initializing Firebase RTDB...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  auth.user.email = fbEmail.c_str();
  auth.user.password = fbPassword.c_str();

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!Firebase.ready()) {
    Serial.println("Firebase not ready. Check API key, DB URL, and credentials.");
    return;
  }

  ensureDBStructure();
  syncRelaysFromRTDB();

  String path = baseSwitchesPath();
  if (!Firebase.RTDB.beginStream(&stream, path.c_str())) {
    Serial.printf("Stream init failed: %s\n", stream.errorReason().c_str());
    return;
  }
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  firebaseInitialized = true;
  Serial.println("RTDB stream listening at: " + path);
}

// ====== RTDB helpers ======
void ensureDBStructure() {
  for (int i = 1; i <= NUM_SWITCHES; i++) {
    String p = switchPath(i);
    if (!Firebase.RTDB.getBool(&fbdo, p.c_str())) {
      if (!Firebase.RTDB.setBool(&fbdo, p.c_str(), false)) {
        Serial.printf("Failed to create %s: %s\n", p.c_str(), fbdo.errorReason().c_str());
      } else {
        Serial.println("Created " + p + " = false");
      }
    }
  }
}

void syncRelaysFromRTDB() {
  for (int i = 1; i <= NUM_SWITCHES; i++) {
    String p = switchPath(i);
    if (Firebase.RTDB.getBool(&fbdo, p.c_str())) {
      bool on = fbdo.to<bool>();
      setRelay(i, on);
      Serial.printf("Sync relay %d -> %s\n", i, on ? "ON" : "OFF");
    } else {
      Serial.printf("Sync read failed %s: %s\n", p.c_str(), fbdo.errorReason().c_str());
    }
  }
}

// ====== Stream callbacks ======
void streamCallback(FirebaseStream data) {
  Serial.println("Stream update detected");
  Serial.printf("Path: %s | Type: %s\n", data.dataPath().c_str(), data.dataType().c_str());
  Serial.printf("Raw: %s\n", data.stringData().c_str());

  String path = data.dataPath(); // e.g. "/switch1/app_on"

  if (path.indexOf("/app_on") > 0 && data.dataTypeEnum() == fb_esp_rtdb_data_type_boolean) {
    int idx = switchIndexFromPath(path.substring(0, path.indexOf("/app_on")));
    if (idx >= 1 && idx <= NUM_SWITCHES) {
      bool on = data.boolData();
      setRelay(idx, on);
      Serial.printf("Applied relay %d -> %s\n", idx, on ? "ON" : "OFF");
    }
    return;
  }

  // fallback for JSON or other types
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
    syncRelaysFromRTDB();
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("Stream timeout, retrying...");
}

// ====== AP Mode + Web UI ======
void startAPMode() {
  isAPMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("AP started. Connect to SSID '%s' and browse http://%s\n", AP_SSID, IP.toString().c_str());

  server.onNotFound([IP]() {
    server.sendHeader("Location", String("http://") + IP.toString(), true);
    server.send(302, "text/plain", "Redirecting to configuration...");
  });

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/toggle", handleToggle);
  server.on("/scan", handleScan);

  server.begin();
  Serial.println("AP web server running.");
}

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Config</title></head><body>
  <h1>ESP32 Configuration</h1>
  )rawliteral";

  html += "<p>Device ID: <code>" + deviceID + "</code></p>";
  html += "<form action=\"/config\" method=\"POST\">";
  html += "WiFi SSID: <input name=\"ssid\" value=\"" + wifiSSID + "\"><br>";
  html += "WiFi Password: <input type=\"password\" name=\"password\"><br>";
  html += "Firebase Email: <input name=\"fb_email\" value=\"" + fbEmail + "\"><br>";
  html += "Firebase Password: <input type=\"password\" name=\"fb_password\"><br>";
  html += "<input type=\"submit\" value=\"Save & Connect\"></form>";
  server.send(200, "text/html", html);
}

void handleConfig() {
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("password");
  fbEmail = server.arg("fb_email");
  fbPassword = server.arg("fb_password");

  preferences.putString("ssid", wifiSSID);
  preferences.putString("password", wifiPassword);
  preferences.putString("fb_email", fbEmail);
  preferences.putString("fb_password", fbPassword);

  server.send(200, "text/html", "<h1>Credentials saved. Restarting...</h1>");
  delay(1500);
  ESP.restart();
}

void handleToggle() {
  int sw = -1;
  if (server.hasArg("switch")) sw = server.arg("switch").toInt();
  if (sw >= 1 && sw <= NUM_SWITCHES) {
    bool newState = !relayIsOn(sw);
    if (!isAPMode && WiFi.status() == WL_CONNECTED && Firebase.ready()) {
      String p = switchPath(sw);
      if (!Firebase.RTDB.setBool(&fbdo, p.c_str(), newState)) {
        Serial.printf("RTDB set failed for %s: %s\n", p.c_str(), fbdo.errorReason().c_str());
        setRelay(sw, newState);
      } else {
        setRelay(sw, newState);
      }
    } else {
      setRelay(sw, newState);
    }
    Serial.printf("Manual toggle switch %d -> %s\n", sw, newState ? "ON" : "OFF");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleScan() {
  String json = "{\"ssids\":[";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    ssid.replace("\"", "\\\"");
    json += "\"" + ssid + "\"";
    if (i < n - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
  WiFi.scanDelete();
}
