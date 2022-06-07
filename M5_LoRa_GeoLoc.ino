int skipToNext(char *, char, int);
String getdms(double, bool);
void checkGPS();
void clearBufferArray();
void discardsCenterCoords();
void doNone();
void drawCenterHeader();
void drawMap();
void drawMenuButtons(uint16_t, uint16_t, uint16_t, uint16_t, String, String, String);
void execComms();
void execLoRaPrefs();
void loadUpToDollar();
void loopPrefs();
void mainMode();
void savePref(char*, uint8_t);
void setupLoRaMenu();
void setupPrefsMenu();
void setupScreen();
void useCenterCoords();

#define ILI9341_DISPON  0x29
#define ILI9341_DISPOFF 0x28
#define TFT_BROWN 0x38E0
#define TFT_GREY 0x7BEF
#define bannerTop 50
#define bannerHeight 40
#define bannerWidth 130
#define lineIncrement 15

uint8_t firstLinePos = bannerTop + bannerHeight + 2;
uint8_t linePos;

#include <esp_sleep.h>
#include <esp_bt_main.h>
#include <sstream> // std::stringstream, std::stringbuf
#include "xbm.h" // Sketch tab header for xbm images
#include <ESP32-Chimera-Core.h>
#include <M5LoRa.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "LoRa_Stuff.h"
#include "HW_AES.h"
#include <M5Widget.h>
#include <Preferences.h>
#include "Prefs.h"
#include "PrefsLoRa.h"

HardwareSerial gpsSerial = HardwareSerial(2);
char buffer[256];
int count = 0;
bool hasFix = false, screenOn = true, mapDrawn = true;
float myLat, myLong;
double lastMapDraw = 0;
String myLatText, myLongText, fixTime, fixDate;
uint8_t zoom = 13; // Set up whatever initial zoom level you like
String COORDS = "";

#include "GPShelper.h"
#include "MainMenu.h"

void availableStats() {
  // LoRa
  linePos = firstLinePos;
  M5.Lcd.drawXBitmap(0, linePos, loraLogo, loraWidth, loraHeight, TFT_CYAN, TFT_WHITE);
  M5.Lcd.setFreeFont(&FreeSans9pt7b);
  M5.Lcd.setTextColor(TFT_BLUE, TFT_WHITE);
  String s = "LoRa: " + String(freq, 3) + " MHz SF: " + String(sfChoice + 6);
  M5.Lcd.drawString(s, 52, linePos); linePos += lineIncrement;
  s = "BW: " + String(bwChoices[bwChoice].c_str());
  s = s + ", CR: 4/" + String(crChoice + 5);
  M5.Lcd.drawString(s, 52, linePos); linePos += lineIncrement;
  M5.Lcd.drawString("* " + fixTime, 2, 194);
}

void minusZoom() {
  if (zoom > 0) zoom -= 1;
  Serial.println("-1 zoom: " + String(zoom));
  M5.update();
  drawMap();
}

void plusZoom() {
  if (zoom < 20) zoom += 1;
  Serial.println("+1 zoom: " + String(zoom));
  M5.update();
  drawMap();
}

void handleScreen() {
  if (screenOn) {
    Serial.println("DISPOFF");
    M5.Lcd.writecommand(ILI9341_DISPOFF);
    M5.Lcd.setBrightness(0);
    screenOn = false;
  } else {
    Serial.println("DISPON");
    M5.Lcd.writecommand(ILI9341_DISPON);
    M5.Lcd.setBrightness(100);
    screenOn = true;
  }
}

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while (count < 30) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

void loraSetup() {
  LoRa.setPins(LORA_CS_PIN, LORA_RST_PIN, LORA_IRQ_PIN); // set CS, reset, IRQ pin
  Serial.print(F("Starting LoRa @ ")); Serial.println(freq, 3);
  if (!LoRa.begin(freq * 1E6)) {
    Serial.println(F("Starting LoRa failed!"));
    M5.Lcd.drawString(F("Starting LoRa failed!"), 24, 57, 1);
    M5.Lcd.drawString(F("Cannot do anything!"), 24, 82, 1);
    while (1);
  }
  // This is option 1
  LoRa.writeRegister(REG_PA_CONFIG, 0b10001111); // That's for the transceiver
  LoRa.writeRegister(REG_PA_DAC, PA_DAC_HIGH); // That's for the transceiver
  //LoRa.writeRegister(REG_LNA, 00); // TURN OFF LNA FOR TRANSMIT
  LoRa.writeRegister(RegBitrateMsb, 0x1a);
  LoRa.writeRegister(RegBitrateLsb, 0x0b); //  . Bitrate: 0b1001010111111, 0x4799
  Serial.println(F("LoRa init succeeded."));

  Serial.println("Setting frequency to " + String(freq, 3));
  LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
  delay(10);
  resetRegModemConfig1();
  delay(10);
  resetRegModemConfig2();
  delay(10);
  resetRegModemConfig3();
  delay(10);
  LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
  Serial.println("LoRa @ " + String(readFreq()) + " MHz");
  displayRegisters();
}

void mainMode() {
  setupScreen();
  uint16_t colour = TFT_RED;
  if (hasFix) colour = TFT_GREEN;
  M5.Lcd.drawXBitmap(270, 0, gpsLogo, gpsWidth, gpsHeight, TFT_WHITE, colour);
  M5.Lcd.drawXBitmap(0, 0, OnAirLogo, gpsWidth, gpsHeight, TFT_WHITE, colour);
  availableStats();
  touch.setTouchFunctionA(setupCommsMenu);
  touch.setTouchFunctionB(setupPrefsMenu);
  mapDrawn = false;
  if (!hasFix) {
    drawMenuButtons(TFT_BROWN, TFT_BLUE, TFT_GREY, TFT_WHITE, "comms", "menu", "[x]");
    touch.setTouchFunctionC(doNone);
  }
  else {
    drawMenuButtons(TFT_BROWN, TFT_BLUE, TFT_GREEN, TFT_WHITE, "comms", "menu", "map");
    touch.setTouchFunctionC(mapMode);
  }
}

void mapMode() {
  if (!hasFix) return;
  drawMap();
  touch.setTouchFunctionC(mainMode);
}

void setupScreen() {
  // Lcd display
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Lcd.fillScreen(TFT_WHITE);
  M5.Lcd.fillRect(0, bannerTop, bannerWidth, bannerHeight, TFT_YELLOW);
  M5.Lcd.fillCircle(bannerWidth, bannerTop + bannerHeight / 2, bannerHeight / 2, TFT_YELLOW);
  M5.Lcd.setFreeFont(&FreeSans12pt7b);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_YELLOW);
  M5.Lcd.drawString("#LoRaBandit", 2, bannerTop + 10);
  M5.Lcd.setFreeFont(&FreeMonoBold9pt7b);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  linePos = firstLinePos;
}

void setup() {
  esp_err_t err = esp_bluedroid_disable();
  err = esp_bluedroid_deinit();
  centerTop = HEADERHEIGHT + 7;
  centerHeight = 240 - (HEADERHEIGHT + FOOTERHEIGHT);
  centerDrawTop = centerTop + HEADERHEIGHT;
  Serial.begin(115200);
  M5.begin();
  Serial.print("\n\n\n");
  lastMapDraw = 0;
  setupScreen();
  M5.Lcd.setFreeFont(&FreeMonoBold9pt7b);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Lcd.drawString("* M5 Serial on", 2, linePos);
  linePos += 14;
  // the gpsSerial baud rate
  //gpsSerial.begin(9600);
  gpsSerial.begin(9600, SERIAL_8N1, 22, 21);
  M5.Lcd.drawString("* GPS Serial on", 2, linePos);
  linePos += 14;
  M5.Lcd.drawXBitmap(270, 0, gpsLogo, gpsWidth, gpsHeight, TFT_WHITE, TFT_RED);
  M5.Lcd.drawString("* Prefs", 2, linePos);
  doPrefs();
  M5.Lcd.drawString("done.", 100, linePos);
  linePos += 14;
  Serial.println("\n\n+-----------------------+");
  Serial.println("+      GPS Tester       +");
  Serial.println("+-----------------------+");
  M5.Lcd.drawString("* Wifi...", 2, linePos);
  //  String wifi_ssid = "Didier S9";
  //  String wifi_password = "wdcw6511";
  wifi_ssid = SSID0;
  wifi_password = PWD0;
  Serial.println("\n\nTrying with main SSID.");
  Serial.print("WIFI-SSID: ");
  Serial.println(wifi_ssid);
  Serial.print("WIFI-PASSWD: ");
  Serial.println(wifi_password);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  M5.Lcd.drawXBitmap(270, 50, signalLogo, signalWidth, signalHeight, TFT_WHITE, TFT_RED);
  bool x = false;
  uint8_t attempts = 0;
  while (!x && attempts < 12) {
    Serial.write(' ');
    x = checkConnection();
    attempts += 1;
  }
  if (!x) {
    wifi_ssid = SSID1;
    wifi_password = PWD1;
    Serial.println("\n\nTrying with alternate SSID!");
    Serial.print("WIFI-SSID: ");
    Serial.println(wifi_ssid);
    Serial.print("WIFI-PASSWD: ");
    Serial.println(wifi_password);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    while (!x) {
      Serial.write(' ');
      x = checkConnection();
    }
  }
  M5.Lcd.drawString("on: " + wifi_ssid, 100, linePos);
  Serial.println(WiFi.localIP());
  M5.Lcd.drawXBitmap(270, 50, signalLogo, signalWidth, signalHeight, TFT_WHITE, TFT_BLUE);
  linePos += 14;

  M5.Lcd.drawXBitmap(270, 100, loraLogo, loraWidth, loraHeight, TFT_RED, TFT_WHITE);
  M5.Lcd.drawString("* LoRa", 0, linePos);
  // LoRa setup
  loraSetup();
  M5.Lcd.drawString("on " + String(freq) + " MHz", 80, linePos);
  linePos += 14;
  M5.Lcd.drawXBitmap(270, 100, loraLogo, loraWidth, loraHeight, TFT_CYAN, TFT_WHITE);

  touch.setTouchFunctionA(setupCommsMenu);
  touch.setTouchFunctionB(setupPrefsMenu);
  touch.setTouchFunctionC(doNone);
  drawMenuButtons(TFT_BROWN, TFT_BLUE, TFT_GREY, TFT_WHITE, "comms", "menu", "[x]");
  while (!hasFix) {
    checkGPS();
    touch.tm.run();
    yield();
  }
  drawMenuButtons(TFT_BROWN, TFT_BLUE, TFT_GREEN, TFT_WHITE, "comms", "menu", "map");
  touch.setTouchFunctionC(mapMode);
  M5.Lcd.drawXBitmap(270, 0, gpsLogo, gpsWidth, gpsHeight, TFT_WHITE, TFT_GREEN);
  M5.Lcd.drawXBitmap(0, 0, OnAirLogo, gpsWidth, gpsHeight, TFT_WHITE, TFT_GREEN);
  M5.Lcd.fillRect(270, 50, 50, 190, TFT_WHITE);
  M5.Lcd.setFreeFont(&FreeMonoBold9pt7b);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Lcd.drawString("* First GPS Fix", 2, linePos);
  linePos += 14;
  M5.Lcd.drawString("Lat: " + myLatText, 2, linePos);
  linePos += 14;
  M5.Lcd.drawString("Long: " + myLongText, 2, linePos);
  /*
    M5.Lcd.fillRoundRect(221, 240 - FOOTERHEIGHT, 60, FOOTERHEIGHT, 3, TFT_RED); // BtnC
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.drawCentreString("map", 221 + 30, 240 - FOOTERHEIGHT + 6, 2);
  */
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  hasFix = true;
}

void loop() {
  touch.tm.run();
  if (gpsSerial.available()) {
    checkGPS();
    if (!mapDrawn) {
      uint16_t colour = TFT_RED;
      if (hasFix) colour = TFT_GREEN;
      M5.Lcd.drawXBitmap(270, 0, gpsLogo, gpsWidth, gpsHeight, TFT_WHITE, colour);
      M5.Lcd.drawXBitmap(0, 0, OnAirLogo, gpsWidth, gpsHeight, TFT_WHITE, colour);
    }
  }
}
