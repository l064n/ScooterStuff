/////////////////////////////////////////////////////////////
// Authors:
// Logan Phillips
// Joey Babcock - https://joeybabcock.me/
/////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <ESP8266WebServer.h>

//OLED libraries
#include "Wire.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define NINEBOT 1
#define BIRD_ZERO 2
#define LIME 3
#define SPIN 4

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int buttonPin = 14;     // the number of the pushbutton pin, D5
const int ledPin =  16;       // the number of the LED pin, D7
const int spinLED = 2;        // LED pin for Spin IOT unit
const int spinSpeaker = 12;   // D6
const int highPin = 15;       //

//Serial Array Input Variables
const byte numChars = 32;
char battString[numChars] = "0000000";   // an array to store the received battery data
boolean newData = false;

//debounce variables
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

//oled display maintaining vars
//int ledStatus = 0;
bool ledSleep = LOW;
int powerOn = 0;            //0 = no model selected, 1 = ON, 2 = OFF

//variables for loop conditions
bool lockStatus = LOW;
bool birdZero = LOW;
int spinLoop = 0;
int limeSJ = 00;
unsigned long lastMessageTime = 0;  // the last time the output pin was toggled
unsigned long lastMessageTime2 = 0;  // the last time the output pin was toggled

int modelNum = 0; //01 = ninebot, 02 = bird_zero, 03 = lime_sj_2.5

//ninebot variables
byte lock01[] = {0x5A, 0xA5, 0x01, 0x3D, 0x20, 0x02, 0x70, 0x01, 0x2E, 0xFF}; 
byte unlock01[] = {0x5A, 0xA5, 0x01, 0x3D, 0x20, 0x02, 0x71, 0x01, 0x2D, 0xFF};
byte readBatt01[] = {0x5A, 0xA5, 0x01, 0x3D, 0x20, 0x01, 0xB4, 0x01, 0xEB, 0xFE}; //read battery percemtage
//String battString = "00000000"; 

//bird_zero variables
byte unlock02_1[] = {0xA6, 0x12, 0x02, 0x10, 0x14, 0xCF}; //what does each command do?
byte unlock02_2[] = {0xA6, 0x12, 0x02, 0x11, 0x14, 0x0B}; 
byte unlock02_3[] = {0xA6, 0x12, 0x02, 0x15, 0x14, 0x30}; 
 
//lime_sj_2.5 variables
byte messageOff03[] = {0x46, 0x43, 0x16, 0x61, 0x00, 0x01, 0xF0, 0xE2, 0xAE}; //If the scooter is on turn it off.
byte messageSpeedLimit[] = {0x46, 0x43, 0x16, 0x11, 0x00, 0x02, 0x00, 0xFF, 0x3A, 0x8B}; //Remove Speed limit
byte messageStart[] = {0x46, 0x43, 0x16, 0x61, 0x00, 0x01, 0xF1, 0xF2, 0x8F}; //This is the unlock code and headlight on.
byte lightOn03[] = {0x46, 0x43, 0x16, 0x12, 0x00, 0x01, 0xF1, 0x2B, 0x26};
byte lightOff03[] = {0x46, 0x43, 0x16, 0x12, 0x00, 0x01, 0xF0, 0x3B, 0x07};
byte lightFlash03[] = {0x46, 0x43, 0x16, 0x13, 0x00, 0x01, 0xFF, 0xBC, 0x5C}; //Flash light on/off for lime sj 2.5
byte lightFlashOff03[] = {0x46, 0x43, 0x16, 0x13, 0x00, 0x01, 0x00, 0xA2, 0xAC};

//Spin ES200B 
byte unlock04[] = {0xA6, 0x12, 0x02, 0xE5, 0xE4, 0xDD}; // LIGHT ON & ESC ON & MPH & MAX SPEED
byte lightOn04[] = {0xA6, 0x12, 0x02, 0x05, 0xE4, 0xA8}; // LIGHT ON & ESC ON & MPH & MAX SPEED
byte lightOff04[] = {0xA6, 0x12, 0x02, 0x01, 0xE4, 0x93}; // LIGHT OFF? & ESC ON & MPH & MAX SPEED
byte lightFlash04[] = {0xA6, 0x12, 0x02, 0x03, 0xE4, 0x02}; // LIGHT FLASH? & ESC ON & MPH & MAX SPEED
//byte unlock04[] = {0xA6, 0x12, 0x02, 0xE5, 0xFF, 0x60}; // LIGHT ON & ESC ON & MPH & MAX SPEED?? 22MPH freewheel, 19.5mph flat
//byte messageStart[] = {0xA6, 0x00, 0x00, 0xF5, 0xFF, 0xFC}; // LIGHT ON & ESC ON & KPH & MAX SPEED & BLINK OFF & TURBO ON & FAST AZ
byte messageOff04[] = {0xA6, 0x12, 0x02, 0x10, 0x14, 0xCF}; //Turns scooter off

/* SSID & Password */
const char* ssid = "Scooter";
const char* password = "12345678";

/* IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(highPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(spinLED, OUTPUT);
  pinMode(spinSpeaker, OUTPUT);
  digitalWrite(ledPin, HIGH);
  digitalWrite(spinLED, LOW);
  digitalWrite(spinSpeaker, HIGH);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  
  server.on("/ninebot", handle_ninebot);
  server.on("/bird_zero", handle_birdZero);
  server.on("/lime_sj", handle_limeSJ);
  server.on("/spin", handle_spin);
  
  server.on("/scooterUnlock", handle_scooterUnlock);
  server.on("/lightOn", handle_lightOn);
  server.on("/lightFlash", handle_lightFlash);
  server.on("/lightOff", handle_lightOff);
  server.on("/scooterLock", handle_scooterLock);
  server.on("/scooterOff", handle_scooterOff);
  server.on("/battLevel", handle_battLevel);
  
  server.begin();
  Serial.println("HTTP server started");

  /*OLED*/
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }
  dispModel(modelNum, powerOn);
}

void loop() {
  server.handleClient();
  //bird zero
  if(birdZero) {
    if ((millis()) > (500 + lastMessageTime)){
      Serial.write(unlock02_3, sizeof(unlock02_3));
      lastMessageTime = millis();
      digitalWrite(highPin, LOW); //Set to Hi-Z not needed after scooter starts4
    }
  }
  //spin
  if(spinLoop > 0) {
    if (spinLoop == 1) {
      if ((millis()) > (500 + lastMessageTime)){
        Serial.write(unlock04, sizeof(unlock04));
        lastMessageTime = millis();
      }
    } else if (spinLoop == 2) {
      if ((millis()) > (500 + lastMessageTime)){
        Serial.write(lightOn04, sizeof(lightOn04));
        lastMessageTime = millis();
      }    
    } else if (spinLoop == 3) {
      if ((millis()) > (500 + lastMessageTime)){
        Serial.write(lightOff04, sizeof(lightOff04));
        lastMessageTime = millis();
      } 
    } else if (spinLoop == 4) {
      if ((millis()) > (500 + lastMessageTime)){
        Serial.write(lightFlash04, sizeof(lightFlash04));
        lastMessageTime = millis();
      }     
    }
    digitalWrite(highPin, LOW); //Set to Hi-Z not needed after scooter starts4
  }
  //lime
  if(limeSJ) {
    if ((millis()) > (1500 + lastMessageTime)){
      lastMessageTime = millis();
      Serial.write(messageStart, sizeof(messageStart));
    }
    if ((millis()) > (1500 + lastMessageTime2)){
      lastMessageTime2 = millis() + 300;     
      Serial.write(messageSpeedLimit, sizeof(messageSpeedLimit));
      //pinMode(highPin, INPUT); //Set to Hi-Z not needed after scooter starts4
    }
  }
  /*DEBOUNCE, BUTTON CODE*/
  // read the state of the switch into a local variable:
  int currentRead = digitalRead(buttonPin);

  // If the switch changed, due to noise or pressing:
  if (currentRead != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed:
    if (currentRead != buttonState) {
      buttonState = currentRead;
      if (buttonState == HIGH) {
        digitalWrite(ledPin, LOW);
        if(ledSleep == HIGH){
          dispModel(modelNum, powerOn);
          ledSleep = LOW;
        }
        else if(powerOn == 1){
          powerOn = 2;
          dispModel(modelNum, powerOn);
        }
        else if(powerOn == 2){
          modelNum = 0;
          powerOn = 0;
          dispModel(modelNum, powerOn);
        }
        else if(modelNum == 4){
          modelNum = 1;
          dispModel(modelNum, powerOn);
        }
        else {
          modelNum++;
          dispModel(modelNum, powerOn);
        }
      } 
      else {
        digitalWrite(ledPin, HIGH);
      }
    }
 }
  /*OLED*/
  if(((millis() - lastDebounceTime) > 2500) && ((millis() - lastDebounceTime) < 5000)
    && modelNum > 0 && powerOn == 0) {
    powerOn = 1;
    dispModel(modelNum, powerOn);
  }
  /*POWER SAVE MODE*/
  if(((millis() - lastDebounceTime) > 30000) && ledSleep == LOW) {
    display.clearDisplay();
    display.display();
    ledSleep = HIGH;
    pinMode(highPin, INPUT);
  }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = currentRead;
}

/*SCOOTER HANDLING METHODS*/
void handle_OnConnect() {
  lockStatus = LOW;
  birdZero = LOW;
  spinLoop = LOW;
  limeSJ = 0;
  modelNum = 0;
  server.send(200, "text/html", SendHTML(lockStatus, battString[7])); 
}

void handle_ninebot() {
  Serial.begin(115200, SERIAL_8N1);
  pinMode(highPin, INPUT);
  modelNum = NINEBOT;
  handle_battLevel();
  //server.send(200, "text/html", SendHTML(lockStatus, battString[7])); 
}

void handle_birdZero() {
  Serial.begin(9600, SERIAL_8N1);
  modelNum = BIRD_ZERO;
  server.send(200, "text/html", SendHTML(lockStatus, battString[7]));
}

void handle_spin() {
  Serial.begin(9600, SERIAL_8N1);
  modelNum = SPIN;
  server.send(200, "text/html", SendHTML(lockStatus, battString[7]));
}

void handle_limeSJ() {
  Serial.begin(9600, SERIAL_8N1);
  modelNum = LIME;
  server.send(200, "text/html", SendHTML(lockStatus, battString[7]));  
}

void handle_scooterUnlock() {
  lockStatus = LOW;
  //ninebot
  if(modelNum == NINEBOT) {
    Serial.write(unlock01, sizeof(unlock01));
    delay(500);
  }
  //bird_zero
  else if(modelNum == BIRD_ZERO) {
    pinMode(highPin, OUTPUT);
    digitalWrite(highPin, HIGH);
    Serial.write(unlock02_1, sizeof(unlock02_1));
    delay(500);
    Serial.write(unlock02_2, sizeof(unlock02_2));
    delay(500);
    birdZero = HIGH;  //continually send unlock through loop()
  }
  //lime
  else if(modelNum == LIME) {
    pinMode(highPin, OUTPUT);
    digitalWrite(highPin, HIGH);
    Serial.write(messageSpeedLimit, sizeof(messageSpeedLimit));
    delay(200);
    Serial.write(messageStart, sizeof(messageStart));
    //lastMessageTime = millis();
    //lastMessageTime2 = millis() + 300;
    delay(500);
    limeSJ = HIGH;  //continually send unlock through loop()
  }
  //spin
  else if(modelNum == SPIN){
    pinMode(highPin, OUTPUT);
    digitalWrite(highPin, HIGH);
    Serial.write(unlock04, sizeof(unlock04));
    delay(1000);
    Serial.write(unlock04, sizeof(unlock04));
    delay(500);
    digitalWrite(spinLED, HIGH);
    spinLoop = 1;  //continually send unlock through loop()
  }
  server.send(200, "text/html", SendHTML(false, battString[7])); 
}

void handle_scooterLock() {
  lockStatus = HIGH;
  //ninebot
  if(modelNum == NINEBOT) {
    Serial.write(lock01, sizeof(lock01));
    delay(500);
  }
  server.send(200, "text/html", SendHTML(true, battString[7])); 
}

void handle_scooterOff() {
  //bird_Zero
  if(modelNum == BIRD_ZERO) {
    birdZero = 0;
  }
  //lime
  if(modelNum == LIME) {
    limeSJ = 0;
    delay(500);
    Serial.write(messageOff03, sizeof(messageOff03));
    Serial.write(messageOff03, sizeof(messageOff03));
    delay(200);
  }
  //spin
  if(modelNum == SPIN) {
    spinLoop = 0;
    delay(200);
    Serial.write(messageOff04, sizeof(messageOff04));
    delay(200);
    digitalWrite(spinLED, LOW);
  }
  server.send(200, "text/html", SendHTML(false, battString[7]));
}

void handle_lightOn() {
  //lime
  if(modelNum == LIME) {
    limeSJ = 0;
    delay(200);
    Serial.write(lightOn03, sizeof(lightOn03));
    delay(200);
    limeSJ = 1;
  }
  //spin
  if(modelNum == SPIN) {
    spinLoop = 2;
  }
  server.send(200, "text/html", SendHTML(false, battString[7]));
}

void handle_lightFlash() {
  //lime
  if(modelNum == LIME) {
    limeSJ = 0;
    delay(200);
    Serial.write(lightFlash03, sizeof(lightFlash03));
    delay(200);
    limeSJ = 1;
  }
  //spin
  if(modelNum == SPIN) {
    spinLoop = 4;
  }
  server.send(200, "text/html", SendHTML(false, battString[7]));
}

void handle_lightOff() {
  //lime
  if(modelNum == LIME) {
    limeSJ = 0;
    delay(200);
    Serial.write(lightFlashOff03, sizeof(lightFlashOff03));
    delay(200);
    Serial.write(lightOff03, sizeof(lightOff03));
    delay(200);
    limeSJ = 1;
  }
  if(modelNum == SPIN) {--
    spinLoop = 3;  //continually send unlock through loop()  
  }
  server.send(200, "text/html", SendHTML(false, battString[7]));
}

void handle_battLevel() {
  //ninebot
  if(modelNum == NINEBOT) {
    Serial.write(readBatt01, sizeof(readBatt01));
    delay(500);
    if(Serial.available()){
        //battString = Serial.readString();
        recvWithEndMarker();
        //Try again if readout is invalid
        if(battString[7] > 100 | battString[7] < 0) {
          //battString = Serial.readString();
          recvWithEndMarker();
        }
    }
  }
  server.send(200, "text/html", SendHTML(lockStatus, battString[7]));
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

//Replace Serial.ReadString with non-blocking function
void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;  
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        if (rc != endMarker) {
            battString[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            battString[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

//OLED DISPLAY METHODS
void dispModel(uint8_t num, uint8_t select) {
    display.clearDisplay(); // Clear display buffer
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.stopscroll();
    display.setCursor(0,0);             // Start at top-left corner
  if(num == 0) {
    display.println(F("Ninebot\nBird Zero\nLime SJ 2.5\nOkai Spin"));
    handle_OnConnect();
  }
  //Ninebot
  else if (num == NINEBOT) {
    if(select == 0){
      display.setTextSize(2);   
      display.println(F("Ninebot"));
      display.setTextSize(1);
      display.println(F("Bird Zero\nLime SJ 2.5"));
    } else if (select == 1) {
      display.setTextSize(2);  
      handle_ninebot();
      String out = "BATTERY: \n";
      out += (int)battString[7]; out += "%";
      display.println(out);
    } else if (select == 2) {
      display.setTextSize(2); 
      display.println(F("\n POWER OFF"));
      handle_scooterOff();
    }
  }
  //Bird Zero
  else if (num == BIRD_ZERO) {
    if(select == 0){
      display.println(F("Ninebot"));
      display.setTextSize(2);   
      display.println(F("Bird Zero"));
      display.setTextSize(1);   
      display.println(F("Lime SJ 2.5"));
    } else if (select == 1) {
      display.setTextSize(2);   
      display.println(F("\n POWER ON"));
      handle_birdZero();  
      handle_scooterUnlock();
    } else if (select == 2) {
      display.setTextSize(2); 
      display.println(F("\n POWER OFF"));
      handle_scooterOff();
    }
  }
  //Lime SJ
  else if (num == LIME) {
    if(select == 0){
      display.println(F("Ninebot\nBird Zero"));
      display.setTextSize(2); 
      display.println(F("Lime SJ 2.5"));
      //display.startscrollright(0x02, 0x03);
      display.setTextSize(1); 
    } else if (select == 1) {
      display.setTextSize(2);   
      display.println(F("\n POWER ON"));
      handle_limeSJ();
      handle_scooterUnlock();
    } else if (select == 2) {
      display.setTextSize(2); 
      display.println(F("\n POWER OFF"));
      handle_scooterOff();
    }
  }
  //Spin
  else if (num == SPIN) {
    if(select == 0){
      display.println(F("Bird Zero\nLime SJ 2.5"));
      display.setTextSize(2); 
      display.println(F("Okai Spin"));
      //display.startscrollright(0x02, 0x03);
      display.setTextSize(1); 
    } else if (select == 1) {
      display.setTextSize(2);   
      display.println(F("\n POWER ON"));
      handle_spin();
      handle_scooterUnlock();
    } else if (select == 2) {
      display.setTextSize(2); 
      display.println(F("\n POWER OFF"));
      handle_scooterOff();
    }
  }
  display.display();
}

String SendHTML(uint8_t lockStat, uint8_t batt){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Scooter Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  
  ptr += ".button {display: block;width: 120px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;";
    ptr += "text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button2 {display: inline;width: 100px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;";
    ptr += "text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
    
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  
  //no model selected
  if(modelNum == 0) {
    ptr +="<h1>ESP8266 Scooter Control</h1>\n";
    ptr +="<a class=\"button button-off\" href=\"/ninebot\">NINEBOT</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/bird_zero\">BIRD ZERO</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/lime_sj\">LIME SJ 2.5</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/spin\">OKAI SPIN</a>\n";   
  }
  
  //ninebot
  if(modelNum == NINEBOT) {
    ptr +="<h1>NINEBOT</h1>\n";
    ptr +="<h2>Battery Level: "; ptr += batt;  ptr +="% </h2>";
    if(lockStat) {
      ptr +="<p>Scooter Status: LOCKED</p><a class=\"button button-off\" href=\"/scooterUnlock\">UNLOCK</a>\n";
    }
    else {
      ptr +="<p>Scooter Status: UNLOCKED</p><a class=\"button button-on\" href=\"/scooterLock\">LOCK</a>\n";
    }
    //ptr +="<a class=\"button button-on\" href=\"/battLevel\">Batt Level</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/\">HOME</a>\n";
  }
  
  //bird_zero
  else if(modelNum == BIRD_ZERO) {
    ptr +="<h1>BIRD ZERO</h1>\n";
    ptr +="<h2>green - tx, yellow - rx, black - gnd, blue - pin 15 (D1)</h2>";
    ptr +="<a class=\"button button-off\" href=\"/scooterUnlock\">UNLOCK</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/\">HOME</a>\n";
  }
  
  //lime
  else if(modelNum == LIME) {
    ptr +="<h1>LIME SJ 2.5</h1>\n";
    ptr +="<h2>green - tx, yellow - rx, black - gnd, blue - pin 15 (D6)</h2>";
    ptr +="<a class=\"button button-off\" href=\"/scooterUnlock\">UNLOCK</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/lightOn\">LIGHT ON</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/lightOff\">LIGHT OFF</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/lightFlash\">LIGHT FLASH</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/scooterOff\">TURN OFF</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/\">HOME</a>\n";
  }
  
  //spin
  else if(modelNum == SPIN) {
    ptr +="<h1>SPIN</h1>\n";
    ptr +="<a class=\"button button-off\" href=\"/scooterUnlock\">UNLOCK</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/lightOn\">LIGHT ON</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/lightOff\">LIGHT OFF</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/lightFlash\">LIGHT FLASH</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/scooterOff\">TURN OFF</a>\n";
    ptr +="<a class=\"button button-off\" href=\"/\">HOME</a>\n";
  }
  
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
