Preferences preferences;
#define BAND 433E6
String CENTERCOORDS = "?";
String SSID0;
String SSID1;
String PWD0;
String PWD1;
String wifi_ssid, wifi_password;
uint8_t BW, SF, CR, CRC = 0, AGC = 0x0c;
// AGC = LowDataRateOptimize Enabled 0b00001000 + AgcAutoOn LNA gain set by the internal AGC loop 0b00000100
float freq;
bool centerCoordsDone = false;
M5Touch touch;
M5StaticMenu sm;

uint8_t fqChoice = 2, nFQ = 5;
std::string fqChoices[5] = {
  "420.0", "430.0", "433.0", "434.0", "470.0"
};
float fqNumChoices[5] = {
  420.0, 430.0, 433.0, 434.0, 470.0
};

uint8_t sfChoice = 6, nSF = 7;
std::string sfChoices[7] = {
  "6", "7", "8", "9", "10", "11", "12"
};
uint8_t sfNumChoices[7] = {
  6, 7, 8, 9, 10, 11, 12
};

uint8_t bwChoice = 7, nBW = 10;
std::string bwChoices[10] = {
  "7.8 KHz", "10.4 KHz", "15.6 KHz", "20.8 KHz", "31.25 KHz", "41.7 KHz", "62.5 KHz", "125.0 KHz", "250.0 KHz", "500.0 KHz"
};
uint8_t bwNumChoices[10] = {
  0b00000000, 0b00010000, 0b00100000, 0b00110000, 0b01000000,  0b01010000, 0b01100000, 0b01110000, 0b10000000, 0b10010000
};

uint8_t crChoice = 0, nCR = 4;
std::string crChoices[4] = {
  "4/5", "4/6", "4/7", "4/8"
};
uint8_t crNumChoices[4] = {
  0b00000010, 0b00000100, 0b00000110, 0b00001000
};

uint8_t ocpChoice = 10, nOCP = 15;
std::string ocpChoices[15] = {
  "100 mA", "110 mA", "120 mA", "130 mA", "140 mA", "150 mA", "160",
  "170 mA", "180 mA", "190 mA", "200 mA", "210 mA", "220 mA", "230 mA", "240 mA"
};
uint8_t ocpNumChoices[15] = {
  11, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 28
};

void savePref(char*, uint8_t);


void doPrefs() {
  // Note: Namespace name is limited to 15 chars.
  preferences.begin("LoRaGeoLoc", false);

  /*
    // let's reset the prefs
    // new format - the indexes are saved, not the indexes
    savePref("freq", 2);
    delay(100);
    savePref("BW", 7);
    delay(100);
    savePref("SF", 6);
    delay(100);
    savePref("CR", 0);
    delay(100);
  */

  fqChoice = preferences.getInt("freq", 2);
  freq = fqNumChoices[fqChoice];
  // bwChoice etc are initialized at startup in PrefsLoRa.h
  bwChoice = preferences.getInt("BW", 7);
  BW = bwNumChoices[bwChoice];
  sfChoice = preferences.getInt("SF", 6);
  SF = sfNumChoices[sfChoice];
  crChoice = preferences.getInt("CR", 0);
  CR = crNumChoices[crChoice];
  AGC = preferences.getInt("AGC", AGC);
  // TODO: add a menu for AGC
  Serial.println("\n\nBW: " + String(BW) + " [" + String(bwChoice) + "]; CR: " + String(CR) + " [" + String(crChoice) + "]; SF: " + String(SF) + " [" + String(sfChoice) + "]");
  Serial.println("freq: " + String(freq / 1e6, 3) + " [" + String(fqChoice) + "]");
  CENTERCOORDS = preferences.getString("center", "?");
  CENTERCOORDS = "22.459824,114.0010048"; //test
  Serial.println("CENTERCOORDS: " + CENTERCOORDS);
  /*
    savePref("freq", fqChoice);
    delay(100);
    savePref("BW", bwChoice);
    delay(100);
    savePref("SF", sfChoice);
    delay(100);
    savePref("CR", crChoice);
    delay(100);
  */

  SSID0 = preferences.getString("ssid0", "?");
  PWD0 = preferences.getString("pwd0", "?");
  SSID1 = preferences.getString("ssid1", "?");
  PWD1 = preferences.getString("pwd1", "?");
  if (SSID0 == "?") {
    // No SSID set. We can't go on without it
    Serial.println("No WiFi settings. Reading /WIFI.JSON from SD card.");
    File file = SD.open("/WIFI.JSON", FILE_READ);
    if (!file) {
      Serial.println("Failed to open WiFi.json file! Aborting!");
      while (1) ;
    }
    size_t fSize = file.size(), bytesRead;
    char json[fSize + 1];
    bytesRead = file.read((uint8_t*)json, fSize);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json);
    const char* ssid0 = root["ssid0"];
    const char* ssid1 = root["ssid1"];
    const char* pwd0 = root["pwd0"];
    const char* pwd1 = root["pwd1"];
    SSID0 = String(ssid0);
    SSID1 = String(ssid1);
    PWD0 = String(pwd0);
    PWD1 = String(pwd1);
    preferences.putString("ssid0", SSID0);
    delay(100);
    preferences.putString("ssid1", SSID1);
    delay(100);
    preferences.putString("pwd0", PWD0);
    delay(100);
    preferences.putString("pwd1", pwd1);
    delay(100);
    wifi_ssid = ssid0;
    wifi_password = pwd0;
  }
  Serial.println("SSID0: " + SSID0 + " :: PWD0: " + PWD0);
  Serial.println("SSID1: " + SSID1 + " :: PWD1: " + PWD1);
  if (CENTERCOORDS != "?") {
    // should we use that?
    Serial.println("Should we use " + CENTERCOORDS + " as center coords?");
    drawMenuButtons(TFT_BLUE, 0, TFT_RED, TFT_WHITE, "yes", "", "no");
    M5.Lcd.setFreeFont(&FreeSans9pt7b);
    M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Lcd.drawCentreString("Found Center Coords:", 160, linePos + 16, 1);
    M5.Lcd.drawCentreString(CENTERCOORDS, 160, linePos + 32, 1);
    M5.Lcd.drawCentreString("Use these?", 160, linePos + 48, 1);
    touch.setTouchFunctionA(useCenterCoords);
    touch.setTouchFunctionB(doNone);
    touch.setTouchFunctionC(discardsCenterCoords);
    centerCoordsDone = false;
    while (!centerCoordsDone) touch.tm.run();
    M5.Lcd.fillRect(0, linePos + 14, 320, 120, TFT_WHITE);
    M5.Lcd.setFreeFont(&FreeMonoBold9pt7b);
    M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  }
}

void useCenterCoords() {
  // nothing to do really, we're all set
  centerCoordsDone = true;
}

void discardsCenterCoords() {
  // keep CENTERCOORDS in prefs but set it for now as "?"
  CENTERCOORDS = "?";
  centerCoordsDone = true;
}


void savePref(char* pref, uint8_t index) {
  preferences.putInt(pref, index);
  Serial.print("Saving "); Serial.print(pref); Serial.print(": "); Serial.println(index);
}
