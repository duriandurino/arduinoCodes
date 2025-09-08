#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Firebase_ESP_Client.h>

// Firebase placeholders
#define FIREBASE_HOST "nexon-cord-default-rtdb.firebaseio.com" // Update to Firestore URL if different
#define API_KEY "AIzaSyAQYUM31DCBeE7kGts-9_6_m3u4ZRMOeiY"
#define PROJECT_ID "nexon-cord" // Verify in Firebase Console
#define DATABASE_ID "(default)" // Typically "(default)" for the default Firestore database

// Relay pins
#define RELAY1_PIN 23
#define RELAY2_PIN 22
#define RELAY3_PIN 21
#define RELAY4_PIN 19

const int NUM_SWITCHES = 4;
const int relayPins[NUM_SWITCHES] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};

// AP mode settings
const char* AP_SSID = "NexonCord-Config";  // User-friendly SSID
const char* AP_PASSWORD = "12345678";

// Web server
WebServer server(80);
Preferences preferences;

// Stored credentials
String wifiSSID;
String wifiPassword;
String fbEmail;
String fbPassword;

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool firebaseInitialized = false;

// Unique device ID
String deviceID;

bool isAPMode = false;

// Polling timer
unsigned long lastFirestoreCheck = 0;
const unsigned long firestoreInterval = 2000; // 2 sec for faster updates

// ---- SETUP ----
void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32... at 03:36 AM PST, September 07, 2025");

  // Init relays
  for (int i = 0; i < NUM_SWITCHES; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // Off
  }
  Serial.println("Relay pins initialized");

  // Load credentials
  preferences.begin("wifi-config", false);
  wifiSSID = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("password", "");
  fbEmail = preferences.getString("fb_email", "");
  fbPassword = preferences.getString("fb_password", "");

  if (wifiSSID == "" || fbEmail == "") {
    Serial.println("No credentials found, starting AP mode");
    startAPMode();
  } else {
    Serial.println("Credentials found:");
    Serial.println("WiFi SSID: " + wifiSSID);
    Serial.println("Firebase Email: " + fbEmail);
    connectToWiFi();
  }
}

// ---- LOOP ----
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

  if (firebaseInitialized && millis() - lastFirestoreCheck > firestoreInterval) {
    lastFirestoreCheck = millis();
    for (int i = 1; i <= NUM_SWITCHES; i++) {
      String documentPath = "devices/" + deviceID + "/switches/switch" + String(i);
      Serial.print("Checking Firestore document: " + documentPath);

      if (Firebase.Firestore.getDocument(&fbdo, PROJECT_ID, DATABASE_ID, documentPath.c_str())) {
        Serial.println(" - Success");
        Serial.print("Raw payload: ");
        Serial.println(fbdo.payload());

        FirebaseJson json;
        if (json.setJsonData(fbdo.payload())) {
          FirebaseJsonData jsonData;
          // Correct path for Firestore nested structure
          if (json.get(jsonData, "fields/app_on/booleanValue")) {
            if (jsonData.type == "boolean") {
              bool state = jsonData.to<bool>();
              Serial.println("Switch " + String(i) + " = " + (state ? "ON" : "OFF"));
              digitalWrite(relayPins[i - 1], state ? LOW : HIGH);
            } else {
              Serial.println("Unexpected type for booleanValue: " + String(jsonData.type));
            }
          } else {
            Serial.println("app_on boolean not found in document");
          }
        } else {
          Serial.println("Failed to parse JSON payload");
        }
      } else {
        Serial.println(" - Failed with HTTP code: " + String(fbdo.httpCode()) + ", Reason: " + fbdo.errorReason());
        if (fbdo.httpCode() == 404) {
          Serial.println("Document not found, creating...");
          FirebaseJson content;
          content.set("fields/app_on/booleanValue", false);
          if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, DATABASE_ID, documentPath.c_str(), content.raw())) {
            Serial.println("Created switch " + String(i) + " with default state: OFF");
            digitalWrite(relayPins[i - 1], HIGH);
          } else {
            Serial.println("Creation failed: " + fbdo.errorReason());
          }
        }
      }
      yield();
    }
  }
}

// ---- WIFI & FIREBASE ----
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected to " + wifiSSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Generate and validate device ID
    String macAddress = WiFi.macAddress();
    Serial.println("Raw MAC Address: " + macAddress);
    macAddress.replace(":", "");
    deviceID = "ESP-" + macAddress;
    Serial.println("Generated Device ID: " + deviceID);

    initializeFirebase();
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi. Starting AP mode...");
    preferences.clear();
    startAPMode();
  }
}

void initializeFirebase() {
  Serial.println("Initializing Firebase...");
  config.api_key = API_KEY;
  auth.user.email = fbEmail.c_str();
  auth.user.password = fbPassword.c_str();
  config.database_url = FIREBASE_HOST; // RTDB URL, Firestore uses projectId/databaseId
  Firebase.begin(&config, &auth);

  // Check if Firebase is ready
  if (Firebase.ready()) {
    Serial.println("Firebase ready and authenticated");
  } else {
    Serial.println("Firebase not ready");
  }
  Firebase.reconnectWiFi(true);
  firebaseInitialized = true; // Set flag after successful init
  Serial.println("Firebase initialized");
}

// ---- AP MODE ----
void startAPMode() {
  isAPMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Handle all requests with captive portal redirect, capturing IP
  server.onNotFound([IP]() {
    server.sendHeader("Location", String("http://") + IP.toString(), true);
    server.send(302, "text/plain", "Redirecting to configuration...");
  });

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/toggle", handleToggle);
  server.on("/scan", handleScan);  // WiFi scan endpoint
  server.begin();
  Serial.println("Web server started in AP mode with captive portal");
}

// ---- WEB HANDLERS ----
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Config</title>
    <style>
      body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; background-color: #f4f4f4; }
      h1 { text-align: center; color: #333; }
      form { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
      label { margin: 10px 0 5px; display: block; color: #555; }
      input, select, button { margin: 10px 0; padding: 10px; width: 100%; max-width: 300px; box-sizing: border-box; font-size: 16px; border: 1px solid #ddd; border-radius: 4px; }
      button { background-color: #4CAF50; color: white; border: none; cursor: pointer; }
      button:hover { background-color: #45a049; }
      .switch-section { margin-top: 20px; }
      .switch { display: flex; justify-content: space-between; align-items: center; margin: 10px 0; background: white; padding: 10px; border-radius: 4px; }
      @media (max-width: 600px) { body { padding: 10px; } input, select, button { font-size: 14px; } }
    </style>
  </head>
  <body>
    <h1>ESP32 Configuration</h1>
    <p>Device ID: )rawliteral" + deviceID + R"rawliteral(</p>
    <p>WiFi Status: )rawliteral" + (WiFi.status() == WL_CONNECTED ? "Connected to " + wifiSSID : "Disconnected") + R"rawliteral(</p>
    <form action="/config" method="POST">
      <label for="ssid">WiFi SSID:</label>
      <select id="ssid" name="ssid">
        <option value=")rawliteral" + wifiSSID + R"rawliteral(" selected>)rawliteral" + wifiSSID + R"rawliteral(</option>
      </select>
      <button type="button" onclick="scanWiFi()">Scan WiFi Networks</button><br>
      <label for="password">WiFi Password:</label>
      <input type="password" id="password" name="password"><br>
      <label for="fb_email">Firebase Email:</label>
      <input type="text" id="fb_email" name="fb_email" value=")rawliteral" + fbEmail + R"rawliteral("><br>
      <label for="fb_password">Firebase Password:</label>
      <input type="password" id="fb_password" name="fb_password"><br>
      <input type="submit" value="Save & Connect">
    </form>
    <div class="switch-section">
      <hr>
      <h2>Switch Test</h2>
)rawliteral";
  for (int i = 1; i <= NUM_SWITCHES; i++) {
    bool state = digitalRead(relayPins[i - 1]) == LOW;
    html += "      <div class=\"switch\">Switch " + String(i) + " (" + (state ? "ON" : "OFF") + "): ";
    html += "<button onclick=\"location.href='/toggle?switch=" + String(i) + "'\">Toggle</button></div>\n";
  }
  html += R"rawliteral(
    </div>
    <script>
      function scanWiFi() {
        fetch('/scan')
          .then(response => response.json())
          .then(data => {
            const select = document.getElementById('ssid');
            select.innerHTML = '';
            data.ssids.forEach(ssid => {
              const option = document.createElement('option');
              option.value = ssid;
              option.textContent = ssid;
              select.appendChild(option);
            });
          })
          .catch(error => console.error('Error:', error));
      }
    </script>
  </body>
  </html>
)rawliteral";
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
  delay(2000);
  ESP.restart();
}

void handleToggle() {
  if (server.hasArg("switch")) {
    int sw = server.arg("switch").toInt() - 1;
    if (sw >= 0 && sw < NUM_SWITCHES) {
      digitalWrite(relayPins[sw], !digitalRead(relayPins[sw]));
      Serial.println("Manually toggled switch " + String(sw + 1));
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// New handler for WiFi scan
void handleScan() {
  String json = "{\"ssids\":[";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    json += "\"" + WiFi.SSID(i) + "\"";
    if (i < n - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
  WiFi.scanDelete(); // Clean up memory
}