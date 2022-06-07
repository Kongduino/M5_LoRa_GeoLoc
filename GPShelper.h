String MapQuestKey = "sTrRhK8yf4yDrB5r2BIGprc3l3bwgbWd";
float LatDec, LongDec;

void clearBufferArray() {
  for (int i = 0; i < count; i++) {
    buffer[i] = NULL;
  } // clear all index of array with command NULL
}

String getdms(double ang, bool isLat = true) {
  bool neg(false);
  if (ang < 0.0) {
    neg = true;
    ang = -ang;
  }
  int deg = (int)ang;
  double frac = ang - (double)deg;
  frac *= 60.0;
  int min = (int)frac;
  frac = frac - (double)min;
  double sec = nearbyint(frac * 600000.0);
  sec /= 10000.0;
  if (sec >= 60.0) {
    min++;
    sec -= 60.0;
  }
  String oss;
  if (neg) oss = "-";
  oss += String(deg) + "d " + String(min) + "' " + String(sec) + "\"";
  if (isLat) {
    if (neg) oss += "S";
    else oss += "N";
  } else {
    if (neg) oss += "W";
    else oss += "E";
  }
  return oss;
}

void loadUpToDollar() {
  char c = gpsSerial.read();
  while (c != '$') {
    c = gpsSerial.read();
  }
}

int skipToNext(char *bf, char x, int pos) {
  int ix = pos;
  while (bf[ix] != x) {
    ix += 1;
  }
  return ix;
}

void checkGPS() {
  loadUpToDollar();
  while (gpsSerial.available()) {
    // reading data into char array
    char c = gpsSerial.read();
    buffer[count++] = c; // write data into array
    if (count == 128) break;
    if (c == 10) break;
    if (c == 13) break;
  }
  buffer[count] = 0;
  if (strncmp (buffer, "GPRMC", 5) == 0 || strncmp (buffer, "GNRMC", 5) == 0) {
    Serial.write("\n$");
    Serial.println(buffer);
    if (strncmp (buffer, "GPRMC,,", 7) == 0 || strncmp (buffer, "GNRMC,,", 7) == 0) {
      // Invalid
      if (hasFix) M5.Lcd.drawXBitmap(270, 0, gpsLogo, gpsWidth, gpsHeight, TFT_WHITE, TFT_RED);
      hasFix = false;
      goto TheEnd;
    }
    int ix = skipToNext(buffer, ',', 0) + 1;
    fixTime = String(buffer[ix++]) + String(buffer[ix++]) + ":" + String(buffer[ix++]) + String(buffer[ix++]) + ":" + String(buffer[ix++]) + String(buffer[ix++]) + " UTC";
    ix = skipToNext(buffer, ',', ix) + 1;
    char c = buffer[ix++];
    if (c != 'A') {
      if (hasFix) M5.Lcd.drawXBitmap(270, 0, gpsLogo, gpsWidth, gpsHeight, TFT_WHITE, TFT_RED);
      hasFix = false;
      goto TheEnd;
    } else {
      ix++;
      Serial.println("  [ok]");
      // Valid
      int yx = skipToNext(buffer, ',', ix);
      int i;
      String s = "";
      for (i = ix; i < yx; i++) s = s + String(buffer[i]);
      float Lat = s.toFloat();
      int DD = Lat / 100;
      float SS = Lat - DD * 100;
      LatDec = DD + SS / 60;
      ix = skipToNext(buffer, ',', yx);
      s = String(buffer[ix]);
      if (s == "S") myLat *= -1;
      Serial.print("Latitude: ");
      myLatText = getdms(LatDec, true);
      Serial.println(myLatText);
      // Longitude
      ix += 3;
      yx = skipToNext(buffer, ',', ix);
      s = "";
      for (i = ix; i < yx; i++) s = s + String(buffer[i]);
      float Long = s.toFloat();
      DD = Long / 100;
      SS = Long - DD * 100;
      LongDec = DD + SS / 60;
      ix = skipToNext(buffer, ',', yx);
      s = String(buffer[ix]);
      if (s == "W") myLong *= -1;
      Serial.println("Longitude: ");
      myLongText = getdms(LongDec, true);
      Serial.println(myLongText);
      ix = skipToNext(buffer, ',', yx);
      yx = ix + 1;
      ix = skipToNext(buffer, ',', yx);
      yx = ix + 1;
      ix = skipToNext(buffer, ',', yx);
      yx = ix + 1;
      ix = skipToNext(buffer, ',', yx);
      yx = ix + 1;
      fixDate = "20" + String(buffer[yx + 4]) + String(buffer[yx + 5]) + "/" + String(buffer[yx + 2]) + String(buffer[yx + 3]) + "/" + String(buffer[yx]) + String(buffer[yx + 1]);
      Serial.println("Fix taken at: " + fixTime + " on " + fixDate);
      hasFix = true;
    }
  }
TheEnd:
  clearBufferArray(); // call clearBufferArray function to clear the storaged data from the array
  count = 0; // set counter of while loop to zero
}

void drawMap() {
  COORDS = String(LatDec, 6) + "," + String(LongDec, 6);
  if (CENTERCOORDS == "?") CENTERCOORDS = COORDS;
  lastMapDraw = millis();
  Serial.println("Displaying map at " + COORDS + " @ zoom " + String(zoom));
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  String request;
  // Using the MapQuest API. Get your own key!
  // https://www.mapquestapi.com/staticmap/v5/map?key=MapQuestKey&center=22.459969,114.00457&size=1280,640&zoom=14&size=@2x&locations=22.469969,114.00|flag-lg-7b0099-ping||22.459969,114.00457|marker-start
  if (CENTERCOORDS == "?" || CENTERCOORDS == COORDS) {
    request = "https://www.mapquestapi.com/staticmap/v5/map?key=" + MapQuestKey + "&center=" + COORDS + "&size=320,260&zoom=" + String(zoom) + "&size=@4x&locations=" + COORDS + "|marker-start&scalebar=true";
  } else {
    request = "https://www.mapquestapi.com/staticmap/v5/map?key=" + MapQuestKey + "&center=" + CENTERCOORDS + "&size=320,260&zoom=" + String(zoom) + "&size=@4x&locations=" + CENTERCOORDS + "|marker-start||" + COORDS + "|flag-lg-7b0099-√&scalebar=true";
  }
  Serial.println(request);
  http.begin(request);
  Serial.print("[HTTP] GET...");
  // start connection and send HTTP header
  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf(" code: %d\n", httpCode);
    // file found at server
    if (httpCode == 200) {
      int ln = http.getSize();
      Serial.println("\nPayload: " + String(ln));
      File file = SD.open("/MAP.JPG", FILE_WRITE);
      if (!file) {
        Serial.println("Failed to open file for writing");
      } else {
        Serial.println("writeToStream (file)");
        http.writeToStream(&file);
        file.close();
        Serial.println("drawJpgFile (file)");
        M5.Lcd.drawJpgFile(SD, "/MAP.JPG", 0, 0, 320, 240);
      }
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error %i: %s\n", httpCode, http.errorToString(httpCode).c_str());
    mainMode(); // go back to main screen
  }
  http.end();
  mapDrawn = true;
}
