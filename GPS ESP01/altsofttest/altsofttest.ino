#include <SoftwareSerial.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

SoftwareSerial mySerial(-1,1);

bool wifiCon = false;
bool sentMessage = false;

unsigned long currmillis;

void setup() {
  Serial.begin(9600);
  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println("failed to connect and timeout occurred");
    delay(3000);
    ESP.reset(); 
    delay(5000);
    wifiCon = false;
  }
  wifiCon = true;
  Serial.println("WiFi connected..");
  delay(2000);
  Serial.println("hello");
}

void loop() {
  currmillis = millis();
  if(currmillis>=(15*60000)&&sentMessage==false){
    Serial.println("Processing Location");
    SerialToGMaps();
  }else if(sentMessage==false){
    Serial.println(currmillis);
  }
}

void SerialToGMaps(){
  while (Serial.available() > 0) {
    String nmeaSentence = Serial.readStringUntil('\n');
    //Serial.println(nmeaSentence);
    if (nmeaSentence.startsWith("$GPGGA")) {
      String parts[15];
      int partIndex = 0;
      int startIndex = 0;
      int endIndex = nmeaSentence.indexOf(',');
      while (endIndex != -1 && partIndex < 15) {
        parts[partIndex] = nmeaSentence.substring(startIndex, endIndex);
        startIndex = endIndex + 1;
        endIndex = nmeaSentence.indexOf(',', startIndex);
        partIndex++;
      }
      
      if (partIndex >= 15) {
        float latitude = parts[2].toFloat();
        float longitude = parts[4].toFloat();
        char latDirection = parts[3].charAt(0);
        char lonDirection = parts[5].charAt(0);
        
        latitude = (int)(latitude / 100) + (latitude - (int)(latitude / 100) * 100) / 60.0;
        longitude = (int)(longitude / 100) + (longitude - (int)(longitude / 100) * 100) / 60.0;
        
        if (latDirection == 'S') {
          latitude = -latitude;
        }
        if (lonDirection == 'W') {
          longitude = -longitude;
        }
        
        Serial.print("Latitude: ");
        Serial.println(latitude, 6);
        Serial.print("Longitude: ");
        Serial.println(longitude, 6);
        
        sendToGoogleMapsAPI(latitude, longitude);
      }
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
    SendMessage(formatted_address);
  } else {
    Serial.println("Failed to connect to Google Maps API");
  }
}

void SendMessage(const char* loc){
  sentMessage = true;
  mySerial.println("AT+CMGF=1");    
  delay(1000);  

  mySerial.println("AT+CMGS=\"+09914452738\"\r"); 
  delay(1000);

  mySerial.println("Time limit of 15mins has been reached!");
  delay(100);

  mySerial.println(loc);
  delay(100);

  mySerial.println((char)26);
  delay(1000);
}