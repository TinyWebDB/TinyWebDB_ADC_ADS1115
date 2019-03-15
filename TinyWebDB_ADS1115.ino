 // Sample Arduino Json Web Client
// Downloads and parse http://jsonplaceholder.typicode.com/users/1
//
// Copyright Benoit Blanchon 2014-2017
// MIT License
//
// Arduino JSON library
// https://bblanchon.github.io/ArduinoJson/
// If you like this project, please add a star!

#include <Adafruit_ADS1015.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <time.h>            
#define JST     3600*9
#define VER    "1.0.0"

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */


#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial
#define ledPin D4

WiFiClient client;

// Set the Screen for the Pulse display
const int WIDTH=100;
const int HEIGHT=64;
const int LENGTH=WIDTH;

const char* resource = "http://noise.uc4.net/";           // http resource
#define MAX_DATA WIDTH*10

int data[MAX_DATA];
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response
const size_t MAX_POST_SIZE = MAX_CONTENT_SIZE + MAX_DATA*7;  // max size of the HTTP POST
//char valuePost[MAX_POST_SIZE];
//char params[MAX_POST_SIZE+48];

const unsigned long BAUD_RATE = 9600;      // serial connection speed
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server

#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

void configModeCallback (WiFiManager *myWiFiManager) {
  USE_SERIAL.println("Entered config mode");
  USE_SERIAL.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  USE_SERIAL.println(myWiFiManager->getConfigPortalSSID());
}

HTTPClient http;

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 OLED(OLED_RESET);

// For the display

int x;
int y[LENGTH];
int starttime;
  
void clearY(){
  for(int i=0; i<LENGTH; i++){
    y[i] = -1;
  }
}

void drawY(){
  OLED.drawPixel(x, y[0], WHITE);
  for(int i=1; i<LENGTH; i++){
    if(y[i]!=-1){
      OLED.drawLine(i-1, y[i-1], i, y[i], WHITE);
    }else{
      break;
    }
  }
}

// Draws the graph ticks for the vertical axis
void drawAxis()
{  
  // graph ticks
  for (int x = 0; x < 2; x++) {
    OLED.drawPixel(x,  0, WHITE);
    OLED.drawPixel(x, 10, WHITE);
    OLED.drawPixel(x, 20, WHITE);
    OLED.drawPixel(x, 30, WHITE);
    OLED.drawPixel(x, 40, WHITE);
    OLED.drawPixel(x, 50, WHITE);  
  }
}

void OLED_init()
{
    OLED.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
    OLED.display(); 
   
    //Add stuff into the 'display buffer'
    OLED.setTextColor(WHITE);
    OLED.setCursor(0,0);
    OLED.println("Build @ " + String( __DATE__ ));
    OLED.println("Module Init...");
    delay(10);

    OLED.display(); //output 'display buffer' to screen  

}

void OLED_message(const char* message)
{
  char buff[24];

  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  while (1500000000 >  time(nullptr)) {
    delay(10);   // waiting time settle
  };
  OLED.clearDisplay();
  OLED.setCursor(0,0);
  // Print the IP address
  OLED.println(WiFi.localIP().toString());
  time_t now = time(nullptr);
  struct tm *tm;    
  tm = localtime(&now);
  sprintf(buff, "%02d/%02d/%02d %02d:%02d:%02d",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec); 
  OLED.println(buff);
  OLED.println("message:");
  OLED.println(message);

  OLED.display(); //output 'display buffer' to screen    
}

void setup() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    USE_SERIAL.begin(BAUD_RATE);
   // USE_SERIAL.setDebugOutput(true);

    OLED_init();

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset settings - for testing
    //wifiManager.resetSettings();
  
    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);
  
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if(!wifiManager.autoConnect()) {
      Serial.println("failed to connect and hit timeout");
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(1000);
    } 
  
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
    digitalWrite(ledPin, LOW);
    
    Serial.println("Getting differential reading from AIN0 (P) and AIN1 (N)");
    Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
    
    // The ADC input range (or gain) can be changed via the following
    // functions, but be careful never to exceed VDD +0.3V max, or to
    // exceed the upper and lower limits if you adjust the input range!
    // Setting these values incorrectly may destroy your ADC!
    //                                                                ADS1015  ADS1115
    //                                                                -------  -------
    ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

    ads.begin();
    
    while (0 == time(nullptr)) {
      delay(10);   // waiting time settle
    };
}



int TinyWebDBWebServiceError(const char* message)
{
    OLED_message(message);
    delay(5000);

    return true;
}

void store_TinyWebDB(const char* tag) {    
    int httpCode;
    char params[64];

    const size_t bufferSize = JSON_ARRAY_SIZE(MAX_DATA) + JSON_OBJECT_SIZE(9);
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.createObject();
    root["Ver"] = VER; 
    root["sensor"] = "ADS1115";
    root["localIP"] = WiFi.localIP().toString();

    time_t now = time(nullptr);
    root["localTime"] = String(now);
    root["Count"] = String(MAX_DATA);

    // we need store MAX_DATA on a array
    JsonArray& sersorData = root.createNestedArray("sersorData");
    for (int u = 0; u < MAX_DATA; u++) {
        sersorData.add(String(data[u]));
    }


    // POST パラメータ作る
    sprintf(params, "tag=%s&value=", tag);
    String valuePost=params;
    root.printTo(valuePost);

    OLED.clearDisplay();
    OLED.setCursor(0,0);
    OLED.println("StoreValue");
    OLED.println("Cunt:" + String(MAX_DATA));
    OLED.println("Size:" + String(bufferSize));
//OLED.println(strJSON.length);
//    OLED.println("Save:" + String(strlen(valuePost)) + "/" + String(sizeof(valuePost)));
    OLED.display(); //output 'display buffer' to screen  
    
    httpCode = TinyWebDBStoreValue(tag, valuePost);

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        OLED.println("POST:" + String(httpCode));

    } else {
        OLED.println(http.errorToString(httpCode).c_str());
    }

    http.end();
    OLED.display(); //output 'display buffer' to screen  

    delay(1000);
}

// ----------------------------------------------------------------------------------------
// Wp TinyWebDB API
// Action        URL                      Post Parameters  Response
// Get Value     {ServiceURL}/getvalue    tag              JSON: ["VALUE","{tag}", {value}]
// ----------------------------------------------------------------------------------------
int TinyWebDBGetValue(const char* tag)
{
    char url[64];

    sprintf(url, "%s%s?tag=%s", resource, "getvalue/", tag);

    // configure targed server and url
    http.begin(url);
    
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
    } else {
        TinyWebDBWebServiceError(http.errorToString(httpCode).c_str());
    }

    http.end();

    sprintf(url, "GET =%d", httpCode);
    OLED_message(url);
    delay(5000);

    if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
        const char * value = payload.c_str();
        TinyWebDBGotValue(tag, value);
    }
    
    return httpCode;
}

int TinyWebDBGotValue(const char* tag, const char* value)
{

    String s = value;
    if(s.compareTo("on")) digitalWrite(ledPin, HIGH);
    else  digitalWrite(ledPin, LOW);
    
    OLED.clearDisplay();
    OLED.setCursor(0,0);
    OLED.println("GetValue");
    OLED.println("Tag :" + String(tag));
    OLED.println("Valu:" + String(value));
    OLED.display(); //output 'display buffer' to screen  
    
    return 0;   
}



void get_TinyWebDB(const char* tag) {    

    TinyWebDBGetValue(tag);

}


// ----------------------------------------------------------------------------------------
// Wp TinyWebDB API
// Action        URL                      Post Parameters  Response
// Store A Value {ServiceURL}/storeavalue tag,value        JSON: ["STORED", "{tag}", {value}]
// ----------------------------------------------------------------------------------------
int TinyWebDBStoreValue(const char* tag, const char* value)
{
    char url[64];
  
    sprintf(url, "%s%s", resource, "storeavalue");

    // POST パラメータ作る
//    sprintf(params, "tag=%s&value=%s", tag, value);

    // configure targed server and url
    http.begin(url);

    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(value);
    
    if(httpCode == HTTP_CODE_OK) {
        http.getString();
//        String payload = http.getString();                  //Get the response payload
        TinyWebDBValueStored();
    } else {
        TinyWebDBWebServiceError(http.errorToString(httpCode).c_str());
    }

    http.end();
    return httpCode;
}

int TinyWebDBValueStored()
{
  
    return 0;   
}


// Parse the JSON from the input string and extract the interesting values
// Here is the JSON we need to parse
// [
//   "VALUE",
//   "LED1",
//   "on",
// ]
bool TinyWebDBreadReponseContent(char* tag, char* value, const char* payload) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // See https://bblanchon.github.io/ArduinoJson/assistant/
  const size_t BUFFER_SIZE =
      JSON_OBJECT_SIZE(3)    // the root object has 3 elements
      + MAX_CONTENT_SIZE;    // additional space for strings

  // Allocate a temporary memory pool
  DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);

  // JsonObject& root = jsonBuffer.parseObject(payload);
  JsonArray& root = jsonBuffer.parseArray(payload);
  JsonArray& root_ = root;

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in
  strcpy(tag, root_[1]);   // "led1"
  strcpy(value, root_[2]); // "on"

  return true;
}


void loop() {
  char  tag[32];

  // Clear the buffer.
  OLED.clearDisplay();
  drawAxis();
  clearY();
  OLED.display();   

  float v0;
  int Signal;
  starttime = millis();
  for (x=0; x< WIDTH; x++) {
    for (int i=0; i<10; i++) {
//        Signal = analogRead(A0);                // Read from A0
//        y[x] = map(Signal, 0, 1550, HEIGHT-14, 0);  
//        delay(9);
      Signal = ads.readADC_SingleEnded(0);    // read from analog-to-digital converter ADS1115
//      v0 = (Signal * 0.1875/1000); // ADS A0 Read
      y[x] = map(Signal, 0, 26666, HEIGHT-14, 0); // Leave some screen for the text.....
      data[x*10+i] = Signal;
    }
    if(x%20 == 0) {
      drawY();
      OLED.setCursor(0,10);
      OLED.print(String(v0));
      OLED.print(" Signal=");
      OLED.print(Signal);
      OLED.display();   
    }
  }
  int endtime = millis()-starttime;
  OLED.clearDisplay();
  drawAxis();
  OLED.drawLine(0, 51, 127, 51, WHITE);
  OLED.drawLine(0, 63, 127, 63, WHITE);
  OLED.setTextSize(0);
  OLED.setTextColor(WHITE);
  OLED.setCursor(0,54);
  OLED.print("Time=");
  OLED.print(endtime);
  OLED.print(" Count=");
  OLED.print(x);
  OLED.print("  ");
  OLED.display();   
  delay(5000);

  sprintf(tag, "roomnoise-%06x", ESP.getChipId());
  digitalWrite(ledPin, HIGH);
  store_TinyWebDB(tag);
  digitalWrite(ledPin, LOW);
  delay(5000);

  sprintf(tag, "led-%06x", ESP.getChipId());
  digitalWrite(ledPin, HIGH);
  get_TinyWebDB(tag);
  digitalWrite(ledPin, LOW);
  delay(5000);

}
