#include <M5Widget.h>

#define HEADERHEIGHT 30
#define FOOTERHEIGHT 28
extern uint16_t centerTop, centerHeight, centerDrawTop;
extern bool mainMenuStay;
extern String cenTerTitle;
extern M5Touch touch;
extern M5StaticMenu sm;

uint8_t pChoice = 0, nPC;
std::string pChoices[20];
uint8_t savedChoice;

void resetRegModemConfig1() {
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
  */
  Serial.println("BW: " + String(BW));
  Serial.println("CR: " + String(CR));
  byte val = BW + CR; // Explicit Header mode 0 by default
  _SPIwrite(REG_MODEM_CONFIG_1, val);
}

void resetRegModemConfig2() {
  /*
    • RegModemConfig2 0x1e
    aaaabcdd
    aaaa: SF 6 to 12 (2^SF chips/symbol)
    b     TxContinuousMode 0: normal mode. 1: continuous mode
    c     RxPayloadCrcOn
    . 0 CRC disabled
    . 1 CRC enabled
    dd    SymbTimeout(9:8) (ignore)
  */
  byte val = SF * 16; // TxContinuous, CRC 0 by default
  _SPIwrite(REG_MODEM_CONFIG_2, val);
}

void resetRegModemConfig3() {
  /*
    • RegModemConfig3 0x26
    0000ab00
    a     LowDataRateOptimize
    . 0 Disabled
    . 1 Enabled; mandated for when the symbol length exceeds 16ms
        rs= BW/2^SF
    b     AgcAutoOn
    . 0 LNA gain set by register LnaGain
    . 1 LNA gain set by the internal AGC loop
  */
  _SPIwrite(REG_MODEM_CONFIG_3, AGC);
}

void drawLoRaHeader() {
  M5.Lcd.fillRect(0, 0, 320, HEADERHEIGHT, TFT_NAVY);
  M5.Lcd.setFont(&FreeSansBold9pt7b);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("LoRa", 1, HEADERHEIGHT - 14, 1);
  M5.Lcd.setFont(&FreeSans9pt7b);
  M5.Lcd.drawString("geotag", 5, 0, 1);
}

void drawLoRaCenter() {
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
  tw = M5.Lcd.textWidth(s, 1);
  M5.Lcd.drawString(s, (320 - tw) / 2, centerDrawTop + 90 , 1);
  M5.Lcd.drawString(s, (320 - tw) / 2 + 1, centerDrawTop + 91 , 1);
  if (pChoice == 0) ix = nPC - 1;
  else ix = pChoice - 1;
  s = String(pChoices[ix].c_str());
  tw = M5.Lcd.textWidth(s, 1);
  M5.Lcd.drawString(s, (320 - tw) / 2, centerDrawTop + 35 , 1);
  M5.Lcd.drawString(s, (320 - tw) / 2 + 1, centerDrawTop + 36 , 1);
}

void drawLoRaFooter() {
  drawMenuButtons(TFT_BLUE, TFT_GREEN, TFT_RED, TFT_WHITE, "+", "OK", "ESC");
  touch.setTouchFunctionA(loopPrefs);
  touch.setTouchFunctionB(execLoRaPrefs);
  touch.setTouchFunctionC(setupPrefsMenu);
}

void execLoRaPrefs() {
  Serial.println("execLoRaPrefs ==> pChoice: " + String(pChoice) + "/" + String(nPC) + " `" + String(pChoices[pChoice].c_str()) + "`");
  switch (savedChoice) {
    case 0: // frequency
      freq = fqNumChoices[pChoice] * 1e6;
      savePref("freq", pChoice);
      break;
    case 1: // BW
      BW = bwNumChoices[pChoice];
      savePref("BW", pChoice);
      LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
      delay(10);
      resetRegModemConfig1();
      delay(10);
      LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
      break;
    case 2: // C/R
      CR = crNumChoices[pChoice];
      savePref("CR", pChoice);
      LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
      delay(10);
      resetRegModemConfig1();
      delay(10);
      LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
      break;
    case 3: // SF
      SF = sfNumChoices[pChoice];
      savePref("SF", pChoice);
      LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
      delay(10);
      resetRegModemConfig2();
      delay(10);
      LoRa.writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
      break;
  }
  pChoice = savedChoice;
  setupPrefsMenu();
}

void setupLoRaMenu() {
  // "frequency", "BW", "C/R", "SF", "AGC", "OCP", "Power"
  cenTerTitle = "";
  Serial.println("setupLoRaMenu ==> pChoice: " + String(pChoice) + "/" + String(nPC));
  savedChoice = pChoice;
  switch (pChoice) {
    case 0: // frequency
      nPC = nFQ;
      pChoice = fqChoice;
      for (uint8_t ix = 0; ix < nPC; ix++) pChoices[ix] = fqChoices[ix];
      cenTerTitle = "Frequency";
      break;
    case 1: // BW
      nPC = nBW;
      pChoice = bwChoice;
      for (uint8_t ix = 0; ix < nPC; ix++) pChoices[ix] = bwChoices[ix];
      cenTerTitle = "Bandwidth";
      break;
    case 2: // C/R
      nPC = nCR;
      pChoice = crChoice;
      for (uint8_t ix = 0; ix < nPC; ix++) pChoices[ix] = crChoices[ix];
      cenTerTitle = "C/R";
      break;
    case 3: // SF
      nPC = nSF;
      pChoice = sfChoice;
      for (uint8_t ix = 0; ix < nPC; ix++) pChoices[ix] = sfChoices[ix];
      cenTerTitle = "Spreading Factor";
      break;
    case 5: // OCP
      nPC = nOCP;
      pChoice = ocpChoice;
      for (uint8_t ix = 0; ix < nPC; ix++) pChoices[ix] = ocpChoices[ix];
      cenTerTitle = "OCP Trim";
      break;
  }
  Serial.println("setupLoRaMenu(`" + cenTerTitle + "`)");
  if (cenTerTitle == "") return;
  M5.Lcd.fillScreen(TFT_WHITE);
  sm.setDrawHeader(drawLoRaHeader);
  sm.setDrawCenter(drawLoRaCenter);
  sm.setDrawFooter(drawLoRaFooter);
  sm.draw();
  while (mainMenuStay) touch.tm.run();
}
