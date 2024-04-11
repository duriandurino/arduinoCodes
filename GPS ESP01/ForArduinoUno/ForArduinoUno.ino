#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define TFT_CS 10
#define TFT_DC 9
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 8
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const int relay = 8;
const int buzzPin = 12;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const byte PASSWORD_LENGTH = 6;
char password[PASSWORD_LENGTH + 1];
byte passwordIndex = 0;
int pwdTries = 0;

int checkFR = 0;
int checkPW = 0;

SoftwareSerial espCam(0, 1);
char readBool;
SoftwareSerial gsm(14, 15);

void setup(){
  Serial.begin(9600);
  espCam.begin(9600);
  gsm.begin(9600);
  pinMode(buzzPin, OUTPUT);
  pinMode(relay, OUTPUT);
  tft.begin();
  tft.setRotation(4);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Enter password: ");
}

void loop(){
  char key = keypad.getKey();

  if(espCam.available()){
    readBool = espCam.read();
    if(readBool == '1'){
      frTries = 0;
      checkFR = 1;
    }else if(readBool == '0'){
      checkPW = 0;
      frTries++;
    }
  }

  if(frTries == 3){
    SendWarning();
    Serial.println("Sent SMS warning");
  }
    else if(frTries == 5){
      CallPolice();
      Serial.println("Calling Police Station");
    }

  if (key != NO_KEY) {
    tone(buzzPin, 330, 150);
    delay(90);
    noTone(buzzPin);
    if (key == '*' || key == '#' || passwordIndex == PASSWORD_LENGTH) {
      // End of password input, validate password
      if (strcmp(password, "123456") == 0) {
        pwdTries = 0;
        checkPW = 1;
        passValid();
        Serial.println("Password Valid!");
      } else {
        pwdTries++;
        checkPW = 0;
        passInvalid();
        Serial.println("Password Invalid!");
      }
      passwordIndex = 0;
      password[0] = '\0';
    } 
    else if(key == 'D'){
        checkPW = 0;
        clearScreen();
        Serial.println("Screen Cleared");
    }
    else {
      // Add key to password array and display "*"
      password[passwordIndex] = key;
      passwordIndex++;
      tft.print("*");
      Serial.println("Taking input from keypad");
    }
  }

  if(pwdTries == 3){
    SendWarning();
    Serial.println("Sent SMS warning");
  }
    else if(pwdTries == 5){
      CallPolice();
      Serial.println("Calling Police Station");
    }

  if((checkFR == 1)&&(checkPW == 1)){
    Serial.println("Access Granted: Door Unlocked");
    digitalWrite(relay, HIGH);
    delay(5000);
  }

  Serial.println("Sleep");
  delay(1000);
}

void passValid(){
  tone(buzzPin, 392, 1000);
  delay(1000);
  noTone(buzzPin);
  tft.setCursor(0, 30);
  tft.println("Access granted!");
  delay(2000);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Enter password:");
}

void passInvalid(){
  tone(buzzPin, 440, 500);
  delay(1000);
  noTone(buzzPin);
  tft.setCursor(0, 30);
  tft.println("Access denied!");
  delay(2000);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Enter password:");
}

void clearScreen(){
  tone(buzzPin, 261, 300);
  delay(100);
  tone(buzzPin, 392, 300);
  delay(100);
  tone(buzzPin, 294, 300);
  delay(100);
  noTone(buzzPin);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Enter password:");
}

void CallPolice() {
  gsm.println("ATD+xxxxxxxxxxxx;"); // ATDxxxxxxxxxx; -- watch out here for semicolon at the end!!
  delay(100);
}

void SendWarning(){
  gsm.println("AT+CMGF=1"); 
  delay(1000);

  gsm.println("AT+CMGS=\"+xxxxxxxxxxx\"\r"); // Replace x with mobile number
  delay(1000);

  gsm.println("INTRUDER ALERT!!! Someone attempted unlocking 3 times!");
  delay(100);

  gsm.println((char)26);
  delay(1000);
}