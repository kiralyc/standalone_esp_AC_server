#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include "M5Atom.h"

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <EasyDDNS.h>

#include <MideaHeatpumpIR.h>  

int scanTime = 1; //In seconds
BLEScan* pBLEScan;

const char* ssid = "";          // Your WiFi SSID
const char* password = "";  // Your WiFi Password
const char* www_username = ""; //your user for basic authentication
const char* www_password = ""; //your password for basic authentication

const char* duckdns_domain = ""; //your domain for duckdns
const char* duckdns_token = ""; //your token for duckdns


const char* ble_device_name = "ATC_XXXXXX"; //your XIAOMI BL thermometer device name after the firmware update it something like this: ATC_123456

int temperature = 1;
int old_temperature = 1;
int humidity = 0;
int old_humidity = 0;
int battery_level = 0;
int battery_voltage = 0;
bool neednotify = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object

AsyncWebSocket ws("/ws");

//init starting values
String message = "";
String sliderValue1 = "22";
String funValue = "2";
String acValue = "3";
String powerValue = "0";

int dutyCycle1;

IRSenderESP32 irSender(25,0);
MideaHeatpumpIR *heatpumpIR;

//Json Variable to Hold Slider Values
JSONVar sliderValues;

//Get Slider Values
String getSliderValues(){
  sliderValues["sliderValue1"] = String(sliderValue1);
  sliderValues["temperature"] = String(temperature / 10.0f);
  sliderValues["humidity"] = String(humidity);
  sliderValues["batterylevel"] = String(battery_level);
  sliderValues["batteryvoltage"] = String(battery_voltage / 1.0e3f);
  sliderValues["fan"] = String(funValue);
  sliderValues["ac"] = String(acValue);
  sliderValues["power"] = String(powerValue);
  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      String stringOne = advertisedDevice.getName().c_str();
      if(advertisedDevice.haveName() && advertisedDevice.haveServiceData() && stringOne.equals(ble_device_name)) {
        neednotify = false;
        uint8_t cServiceData[50];
        uint8_t* payloadPtr = advertisedDevice.getPayload();
        old_temperature = temperature;
        old_humidity = humidity;
        for (int i = 0; i < advertisedDevice.getPayloadLength(); i++)
        {
            cServiceData[i] = *(payloadPtr + i);
            
        }
        temperature = uint16_t(cServiceData[11]) | (uint16_t(cServiceData[10]) << 8);
        if(temperature != old_temperature)
        {
          Serial.println("temperature not equal");
          neednotify = true;
        }
        humidity = uint8_t(cServiceData[12]);
        if(humidity != old_humidity)
        {
          Serial.println("humidity not equal");
          neednotify = true;
        }
        battery_level = uint8_t(cServiceData[13]);
        battery_voltage = uint16_t(cServiceData[15]) | (uint16_t(cServiceData[14]) << 8);
        
        Serial.printf("temperature_or: %.1f °C \n", temperature / 10.0f);
        Serial.printf("Humidity: %u \n", humidity);
        Serial.printf("Battery level: %u \n", battery_level);
        Serial.printf("Battery voltage or: %.1f V \n", battery_voltage / 1.0e3f);
        if(neednotify)
        {
          notifyClients(getSliderValues());
        }
      }
    }  
};

// Initialize SPIFFS
void initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
   Serial.println("SPIFFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sliderValues) {
  ws.textAll(sliderValues);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    if (message.indexOf("1s") >= 0) {
      sliderValue1 = message.substring(2);
      dutyCycle1 = sliderValue1.toInt();
      Serial.println(dutyCycle1);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
      sendCommandOnIr();
    }
    if (message.indexOf("2s") >= 0) {
      funValue = message.substring(2);     
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
      sendCommandOnIr();
    }    
    if (message.indexOf("3s") >= 0) {
      acValue = message.substring(2);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
      sendCommandOnIr();
    }
    if (message.indexOf("4s") >= 0) {
      powerValue = message.substring(2);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
      sendCommandOnIr();
    }
    if (strcmp((char*)data, "getValues") == 0) {
      notifyClients(getSliderValues());
    }
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void sendCommandOnIr() {
  Serial.print("sendCommandOnIr");
  heatpumpIR->send(irSender, powerValue.toInt(), acValue.toInt(), funValue.toInt(), dutyCycle1, VDIR_AUTO, HDIR_AUTO);
}
void setup() {
  Serial.begin(115200);
  M5.begin(true, false, true);
  initFS();
  initWiFi();

  initWebSocket();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->authenticate(www_username, www_password)) {
      Serial.print(F("NOT AUTHENTICATE!"));
      request->requestAuthentication();
    }
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  EasyDDNS.service("duckdns");
  EasyDDNS.client(duckdns_domain, duckdns_token);

   EasyDDNS.onUpdate([&](const char* oldIP, const char* newIP){
    Serial.print("EasyDDNS - IP Change Detected: ");
    Serial.println(newIP);
  });
  heatpumpIR = new MideaHeatpumpIR();
}

void loop() {
  if((powerValue == "1") && (acValue == "3"))
  {
    //AC is ON and it is COOL mode
    M5.dis.drawpix(0, CRGB::Blue);
  }
  if((powerValue == "1") && (acValue == "4"))
  {
    //AC is ON and it is DRY mode
    M5.dis.drawpix(0, CRGB::Green);
  }
  if((powerValue == "1") && (acValue == "5"))
  {
    //AC is ON and it is FAN ONLY mode
    M5.dis.drawpix(0, CRGB::Yellow);
  }
  if((powerValue == "1") && (acValue == "1"))
  {
    //AC is ON and it is AUTO mode
    M5.dis.drawpix(0, CRGB::Purple);
  }
  if((powerValue == "1") && (acValue == "2"))
  {
    //AC is ON and it is HEAT mode
    M5.dis.drawpix(0, CRGB::Red);
  }
  if(powerValue == "0")
  {
    //AC is OFF
    M5.dis.drawpix(0, CRGB::Black);
  }
  delay(50);
  M5.update();
  
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print(F("Devices found: "));
  Serial.println(foundDevices.getCount());
  Serial.println(F("Scan done!"));
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  EasyDDNS.update(10000);
  ws.cleanupClients();
}
