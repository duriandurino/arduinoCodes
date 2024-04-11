//Created on March 23, 2024
//Completed on ??
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>

TinyGPSPlus gps;

unsigned long currMillis;
unsigned long startMillis;
const unsigned long limit = 1*60000;
bool wifiCon = false;

String inputString = ""; // a string to hold incoming data
boolean stringComplete = false; // whether the string is complete
String signal = "$GPGLL";

void setup() {
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  //Serial.println("start");
  if(!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println("failed to connect and timeout occurred");
    delay(3000);
    ESP.reset(); 
    delay(5000);
    wifiCon = false;
  }
  wifiCon = true;
  Serial.println("WiFi connected..");
  Serial.println("setuperist");
  inputString.reserve(200);
}

void loop() {
  if (stringComplete) {
    //Serial.println("convert");
        String BB = inputString.substring(0, 6);
        if (BB == signal) {
            String LAT = inputString.substring(7, 17);
            int LATperiod = LAT.indexOf('.');
            int LATzero = LAT.indexOf('0');
            if (LATzero == 0) {
                LAT = LAT.substring(1);
            }

            String LON = inputString.substring(20, 31);
            int LONperiod = LON.indexOf('.');
            int LONTzero = LON.indexOf('0');
            if (LONTzero == 0) {
                LON = LON.substring(1);
            }

            Serial.println(LAT);
            Serial.println(LON);

        }

        // Serial.println(inputString);
        // clear the string:
        inputString = "";
        stringComplete = false;
    }

    serialEvent();
  //Serial.println("get data done");
  //Serial.println("loop again");
}

void test(){
  // Serial.println(gpsGsm.read());
  // Serial.println(gpsGsm.readStringUntil('\n'));
  // Serial.println("Hello");
}

void serialEvent() {
    while (Serial.available()) {
        // get the new byte:
        char inChar = (char) Serial.read();
        // add it to the inputString:
        inputString += inChar;
        // if the incoming character is a newline, set a flag
        // so the main loop can do something about it:
        if (inChar == '\n') {
            stringComplete = true;
        }
    }
}

void sendToGoogleMapsAPI(float lat, float lon){
  String url = "https://maps.googleapis.com/maps/api/geocode/json?latlng=";
  url += String(lat, 6);
  url += ",";
  url += String(lon, 6);
  url += "&key=";
  url += "AIzaSyA9ZZX8cFJfiBEYR1_MuLzYGhfE6907QM0";
  Serial.print("Sending HTTP request to: ");
  Serial.println(url);
  WiFiClient client;
  if (client.connect("maps.googleapis.com", 443)) { 
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: maps.googleapis.com\r\n" +
                 "Connection: close\r\n\r\n");
    delay(500); 
    String response = "";
    while (client.available()) {
      response += client.readString();
    }
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    const char* formatted_address = doc["results"][0]["formatted_address"];
    Serial.print("Formatted Address: ");
    Serial.println(formatted_address);
  } else {
    Serial.println("Failed to connect to Google Maps API");
  }
}