#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>

MDNSResponder mdns;
SoftwareSerial gsm(3,1);
ESP8266WebServer server(80);

const char* ssid = "PLDTHOMEFIBR8f318";
const char* password = "PLDTWIFInze6y";
const char* host = "maps.googleapis.com";
const char* apiKey = "AIzaSyA9ZZX8cFJfiBEYR1_MuLzYGhfE6907QM0";

void setup() {
  Serial.begin(115200);
  
  delay(10);
  WiFi.mode(WIFI_AP);
  Serial.println();
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  mdns.begin("esp8266", WiFi.localIP());
  Serial.print(WiFi.localIP());
  Serial.println("");
  Serial.println("WiFi connected");
}

void loop() {

  String response;
  float latitude;
  float longitude;

  char latString = latitude;
  char longString = longitude;

  server.handleClient();
  if (gsm.available() > 0) {
    Serial.println("Serial is available");
    String gpsData = gsm.readStringUntil('\n');

    String latitudeStr = gpsData.substring(20, 29); 
    String longitudeStr = gpsData.substring(31, 41); 
    latitude = latitudeStr.toFloat(); 
    longitude = longitudeStr.toFloat(); 
    // Make HTTP request to Google Maps Geocoding API
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("Connection failed");
      return;
    }
    String url = "/maps/api/geocode/json?latlng=" + String(latitude) + "," + String(longitude) + "&key=" + String(apiKey);
    Serial.print("Requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    while (client.connected()) {
      String line = client.readStringUntil('\r');
      if (line == "\n") {
        break;
      }
    }
    // Read response
    while (client.available()) {
      String line = client.readStringUntil('\r');
      response += line;
    }
    Serial.println("Response:");
    Serial.println(response);
  }

  if(millis()==(1*60000)){
    SendWarning(response, latString, longString);
  }
}

void SendWarning(String response, char lat, char longi){
  Serial.println("SENT SMS");
  Serial.println("Location: "+response);
  Serial.println("Latitude: "+lat);
  Serial.println("Longitude: "+longi);

  gsm.println("AT+CMGF=1"); 
  delay(1000);

  gsm.println("AT+CMGS=\"+09914452738\"\r"); 
  delay(1000);

  gsm.println("Warning! 15 minutes have passed but the ferson is not balik parin");
  delay(100);

  gsm.println("Location: "+response);
  delay(100);

  gsm.println("Latitude: "+lat);
  delay(100);

  gsm.println("Longitude: "+longi);
  delay(100);

  gsm.println((char)128);
  delay(1000);
}

void handleRoot() {
  String message = "<html><body>";
  message += "<h1>Enter SSID and Password:</h1>";
  message += "<form action='/setcredentials' method='get'>";
  message += "<input type='text' name='ssid' placeholder='SSID'>";
  message += "<input type='password' name='password' placeholder='Password'>";
  message += "<input type='submit' value='Submit'>";
  message += "</form></body></html>";
  server.send(200, "text/html", message);
}

void handleSetCredentials() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid").c_str();
    password = server.arg("password").c_str();
    Serial.print("SSID set to: ");
    Serial.println(ssid);
    Serial.print("Password set to: ");
    Serial.println(password);
    server.send(200, "text/plain", "Credentials set successfully");
  } else {
    server.send(400, "text/plain", "Incomplete credentials");
  }
}
