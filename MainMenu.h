#define HEADERHEIGHT 30
#define FOOTERHEIGHT 28
uint16_t centerTop, centerHeight, centerDrawTop;
String cenTerTitle = "Main Menu";
bool mainMenuStay = true;
std::string prefsChoices[7] = {
  "frequency", "BW", "C/R", "SF", "AGC", "OCP", "Power"
};
/*

  • RegModemConfig1 0x1d
  aaaabbbc
  aaaa  BW:
    . 0b0000   7.8  KHz
    . 0b0001  10.4  KHz
    . 0b0010  15.6  KHz
    . 0b0011  20.8  KHz
    . 0b0100  31.25 KHz
    . 0b0101  41.7  KHz
    . 0b0110  62.5  KHz
    . 0b0111 125.0  KHz
    . 0b1000 250.0  KHz
    . 0b1001 500.0  KHz
  bbb   CodingRate:
    . 001 4/5
    . 010 4/6
    . 011 4/7
    . 100 4/8
  c     ImplicitHeaderModeOn
    . 0 Explicit Header mode
    . 1 Implicit Header mode

  • RegModemConfig2 0x1e
  aaaabcdd
  aaaa: SF 6 to 12 (2^SF chips/symbol)
  b     TxContinuousMode 0: normal mode. 1: continuous mode
  c     RxPayloadCrcOn
    . 0 CRC disabled
    . 1 CRC enabled
  dd    SymbTimeout(9:8) (ignore)

  • RegModemConfig3 0x26
  0000ab00
  a     LowDataRateOptimize
    . 0 Disabled
    . 1 Enabled; mandated for when the symbol length exceeds 16ms
        rs= BW/2^SF
  b     AgcAutoOn
    . 0 LNA gain set by register LnaGain
    . 1 LNA gain set by the internal AGC loop

  • RegPaConfig 0x09
  abbbcccc
  a     PaSelect:
    . 0 RFO pin. Output power is limited to +14 dBm.
    . 1 PA_BOOST pin. Output power is limited to +20 dBm
  bbb   MaxPower: Select max output power: Pmax=10.8+0.6*MaxPower [dBm]
  cccc  OutputPower
    . if PaSelect = 0 (RFO pin): Pout=Pmax-(15-OutputPower)
    . if PaSelect = 1 (PA_BOOST pin): Pout=17-(15-OutputPower)

  • RegOcp 0x0b
  00abbbbb
  a     OCP ON (1) OFF (0)
  bbbbb OCPTrim
    . if OCPTrim < 16: Imax = 45+5*OcpTrim [mA] (120 mA)
    . if 15 < OCPTrim < 28: Imax = -30+10*OcpTrim [mA] (130 to 240 mA)
    . else Imax = 240mA
*/

void doNone() {}

void drawMenuButtons(
  uint16_t btnAcolor = TFT_BLUE, uint16_t btnBcolor = TFT_OLIVE, uint16_t btnCcolor = TFT_RED, uint16_t textColor = TFT_WHITE,
  String btnAlabel = "[x]", String btnBlabel = "[x]", String btnClabel = "[x]"
) {
  M5.Lcd.fillRect(0, 240 - FOOTERHEIGHT, 320, FOOTERHEIGHT, TFT_WHITE);
  M5.Lcd.setFont(&FreeSans9pt7b);
  if (btnAlabel != "") {
    M5.Lcd.setTextColor(textColor, btnAcolor);
    M5.Lcd.fillRoundRect(31, 240 - FOOTERHEIGHT, 60, FOOTERHEIGHT, 3, btnAcolor);
    M5.Lcd.drawCentreString(btnAlabel, 31 + 30, 240 - FOOTERHEIGHT + 6, 1);
  }
  if (btnBlabel != "") {
    M5.Lcd.setTextColor(textColor, btnBcolor);
    M5.Lcd.fillRoundRect(126, 240 - FOOTERHEIGHT, 60, FOOTERHEIGHT, 3, btnBcolor);
    M5.Lcd.drawCentreString(btnBlabel, 126 + 30, 240 - FOOTERHEIGHT + 6, 1);
  }
  if (btnClabel != "") {
    M5.Lcd.setTextColor(textColor, btnCcolor);
    M5.Lcd.fillRoundRect(221, 240 - FOOTERHEIGHT, 60, FOOTERHEIGHT, 3, btnCcolor);
    M5.Lcd.drawCentreString(btnClabel, 221 + 30, 240 - FOOTERHEIGHT + 6, 1);
  }
}

void drawCenterHeader() {
  M5.Lcd.fillRoundRect(3, centerTop, 314, HEADERHEIGHT, 4, TFT_CYAN);
  M5.Lcd.setFont(&FreeSansOblique12pt7b);
  uint16_t tw = M5.Lcd.textWidth(cenTerTitle);
  uint16_t px = (M5.Lcd.width() / 2) - (tw / 2);
  M5.Lcd.setTextColor(TFT_NAVY);
  M5.Lcd.drawString(cenTerTitle, px, HEADERHEIGHT + 11, 1);
}

void drawPill(uint16_t c) {
  M5.Lcd.fillRoundRect(290, 2, 12, HEADERHEIGHT - 4, 4, c);
  M5.Lcd.drawRoundRect(290, 2, 12, HEADERHEIGHT - 4, 4, TFT_WHITE);
  M5.Lcd.drawRoundRect(291, 3, 10, HEADERHEIGHT - 6, 4, TFT_BLACK);
}

void drawMainHeader() {
  M5.Lcd.fillRect(0, 0, 320, HEADERHEIGHT, TFT_NAVY);
  M5.Lcd.setFont(&FreeSansBold9pt7b);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("LoRa", 1, HEADERHEIGHT - 14, 1);
  M5.Lcd.setFont(&FreeSans9pt7b);
  M5.Lcd.drawString("geotag", 5, 0, 1);
}

void drawMainCenter() {
  M5.Lcd.fillRoundRect(3, centerTop, 314, centerHeight - 14, 4, TFT_WHITE);
  drawCenterHeader();
  M5.Lcd.setTextColor(TFT_CYAN);
  String s;
  s = String(pChoices[pChoice].c_str());
  uint16_t tw = M5.Lcd.textWidth(s, 1);
  M5.Lcd.drawString(s, (320 - tw) / 2, centerDrawTop + 60 , 1);
  M5.Lcd.drawString(s, (320 - tw) / 2 + 1, centerDrawTop + 61 , 1);

  M5.Lcd.setFont(&FreeSansOblique9pt7b);
  M5.Lcd.setTextColor(TFT_LIGHTGREY);
  uint8_t ix = pChoice + 1;
  if (ix == nPC) ix = 0;
  s = String(pChoices[ix].c_str());
  if (prefsChoices[ix].find("!") == 0) {
    // disabled function. remove ! and displayed greyed out
    s = pChoices[ix].substr(1).c_str();
    M5.Lcd.setFont(&FreeMono9pt7b);
  }
  tw = M5.Lcd.textWidth(s, 1);
  M5.Lcd.drawString(s, (320 - tw) / 2, centerDrawTop + 90 , 1);
  M5.Lcd.drawString(s, (320 - tw) / 2 + 1, centerDrawTop + 91 , 1);
  M5.Lcd.setFont(&FreeSansOblique9pt7b);
  if (pChoice == 0) ix = nPC - 1;
  else ix = pChoice - 1;
  s = String(pChoices[ix].c_str());
  if (pChoices[ix].find("!") == 0) {
    // disabled function. remove ! and displayed greyed out
    s = pChoices[ix].substr(1).c_str();
    M5.Lcd.setFont(&FreeMono9pt7b);
  }
  tw = M5.Lcd.textWidth(s, 1);
  M5.Lcd.drawString(s, (320 - tw) / 2, centerDrawTop + 35 , 1);
  M5.Lcd.drawString(s, (320 - tw) / 2 + 1, centerDrawTop + 36 , 1);
  M5.Lcd.setFont(&FreeSansOblique9pt7b);
}

void drawMainFooter() {
  drawMenuButtons(TFT_BLUE, TFT_GREEN, TFT_RED, TFT_WHITE, "+", "OK", "ESC");
  touch.setTouchFunctionA(loopPrefs);
  touch.setTouchFunctionB(setupLoRaMenu);
  touch.setTouchFunctionC(mainMode);
}

void loopPrefs() {
  pChoice++;
  if (pChoice == nPC) pChoice = 0;
  if (pChoices[pChoice].find("!") == 0) loopPrefs();
  else {
    sm.draw();
    Serial.println("loopPrefs ==> pChoice: " + String(pChoice) + "/" + String(nPC));
  }
}

void setupPrefsMenu() {
  mainMenuStay = true;
  M5.Lcd.fillScreen(TFT_WHITE);
  cenTerTitle = "Preferences";
  nPC = 7;
  for (uint8_t ix = 0; ix < nPC; ix++) pChoices[ix] = prefsChoices[ix];
  sm.setDrawHeader(drawMainHeader);
  sm.setDrawCenter(drawMainCenter);
  sm.setDrawFooter(drawMainFooter);
  sm.draw();
  mainMenuStay = true;
  while (mainMenuStay) touch.tm.run();
}

void drawCommsFooter() {
  drawMenuButtons(TFT_BLUE, TFT_GREEN, TFT_RED, TFT_WHITE, "+", "OK", "ESC");
  touch.setTouchFunctionA(loopPrefs);
  touch.setTouchFunctionB(execComms);
  touch.setTouchFunctionC(mainMode);
}

void setupCommsMenu() {
  mainMenuStay = true;
  M5.Lcd.fillScreen(TFT_WHITE);
  cenTerTitle = "Comms";
  nPC = 7;
  std::stringstream ss;
  ss << "W: " << SSID0;
  if (wifi_ssid == SSID0) {
    ss << " [v]";
    pChoice = 0;
  } pChoices[0] = ss.str();

  ss.str("");
  ss << "W: " << SSID1;
  if (wifi_ssid == SSID1) {
    ss << " [v]";
    pChoice = 1;
  } pChoices[1] = ss.str();
  pChoices[2] = "sleep";
  if (hasFix) pChoices[3] = "center here";
  else pChoices[3] = "!center here";
  sm.setDrawHeader(drawMainHeader);
  sm.setDrawCenter(drawMainCenter);
  sm.setDrawFooter(drawCommsFooter);
  sm.draw();
  mainMenuStay = true;
  while (mainMenuStay) touch.tm.run();
  mainMode();
}

void execComms() {
  Serial.println("In execComms");
  std::stringstream ss;
  switch (pChoice) {
    case 0:
      ss << "W: " << SSID0 << " [v]";
      pChoices[0] = ss.str();
      ss.str("");
      ss << "W: " << SSID1;
      pChoices[1] = ss.str();
      wifi_ssid = SSID0;
      wifi_password = PWD0;
      sm.draw();
      // need to reboot
      ESP.restart();
    case 1:
      ss << "W: " << SSID1 << " [v]";
      pChoices[1] = ss.str();
      ss.str("");
      ss << "W: " << SSID0;
      pChoices[0] = ss.str();
      wifi_ssid = SSID1;
      wifi_password = PWD1;
      sm.draw();
      ESP.restart();
  }
}
