// ****************************************************************
// Sketch Esp8266 Admin Modular(Tab)
// created: Jens Fleischer, 2019-12-17
// last mod: Jens Fleischer, 2020-02-17
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.6.0 / 2.6.1 / 2.6.2 / 2.6.3
// Geprüft: von 1MB bis 16MB Flash
// Getestet auf: Nodemcu, Wemos D1 Mini Pro, Sonoff Switch, Sonoff Dual
/******************************************************************
  Copyright (c) 2019 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von Admin sollte als Tab eingebunden werden.
// #include <FS.h> #include <ESP8266WebServer.h> müssen im Haupttab aufgerufen werden
// Die Funktionalität des ESP8266 Webservers ist erforderlich.
// Die Spiffs.ino muss im ESP8266 Webserver enthalten sein
// Funktion "admin();" muss im setup() nach spiffs() aber vor dem Verbindungsaufbau aufgerufen werden.
// Die Funktion "runtime();" muss mindestens zweimal innerhalb 49 Tage aufgerufen werden.
// Entweder durch den Client(Webseite) oder zur Sicherheit im "loop();"
/**************************************************************************************/

const char* const PROGMEM flashChipMode[] = {"QIO", "QOUT", "DIO", "DOUT", "Unbekannt"};

void admin() {                          // Funktionsaufruf "admin();" muss im Setup eingebunden werden
  File file = SPIFFS.open("/config.json", "r");
  if (file) {
    String newhostname = file.readStringUntil('\n');
    if (newhostname != "") {
      WiFi.hostname(newhostname.substring(1, newhostname.length() - 1));
      file.close();
      ArduinoOTA.setHostname(WiFi.hostname().c_str());
    }
  }
  server.on("/admin/renew", handlerenew);
  server.on("/admin/once", handleonce);
  server.on("/reconnect", []() {
    server.send(204, "", "");
    WiFi.reconnect();
  });
  server.on("/restart", []() {
    server.send(204, "", "");
    //speichern();      //Wenn Werte vor dem Neustart gespeichert werden sollen
    ESP.restart();
  });
}

//Es kann entweder die Spannung am ADC-Pin oder die Modulversorgungsspannung (VCC) ausgegeben werden.

void handlerenew() {    // Um die am ADC-Pin anliegende externe Spannung zu lesen, verwende analogRead (A0)
  server.send(200, "application/json", "[\"" + runtime() + "\",\"" + WiFi.RSSI() + "\",\"" + analogRead(A0) + "\"]");     // Json als Array
}
/*
  ADC_MODE(ADC_VCC);
  void handlerenew() {   // Zum Lesen der Modulversorgungsspannung (VCC), verwende ESP.getVcc()
  server.send(200, "application/json", "[\"" + runtime() + "\",\"" + WiFi.RSSI() + "\",\"" + ESP.getVcc() + "\"]");     // Json als Array
  }
*/
void handleonce() {
  if (server.arg(0) != "") {
    WiFi.hostname(server.arg(0));
    File f = SPIFFS.open("/config.json", "w");                    // Datei zum schreiben öffnen
    f.printf("\"%s\"\n", WiFi.hostname().c_str());
    f.close();
  }
  String temp = "{\"File\":\"" + sketchName() + "\", \"Build\":\"" + __DATE__ + " " + __TIME__ + "\", \"SketchSize\":\"" + formatBytes(ESP.getSketchSize()) +
                "\", \"SketchSpace\":\"" + formatBytes(ESP.getFreeSketchSpace()) + "\", \"LocalIP\":\"" +  WiFi.localIP().toString() +
                "\", \"Hostname\":\"" + WiFi.hostname() + "\", \"SSID\":\"" + WiFi.SSID() + "\", \"GatewayIP\":\"" +  WiFi.gatewayIP().toString() +
                "\", \"Channel\":\"" +  WiFi.channel() + "\", \"MacAddress\":\"" +  WiFi.macAddress() + "\", \"SubnetMask\":\"" +  WiFi.subnetMask().toString() +
                "\", \"BSSID\":\"" +  WiFi.BSSIDstr() + "\", \"ClientIP\":\"" + server.client().remoteIP().toString() + "\", \"DnsIP\":\"" + WiFi.dnsIP().toString() +
                "\", \"ResetReason\":\"" + ESP.getResetReason() + "\", \"CpuFreqMHz\":\"" + F_CPU / 1000000 + "\", \"FreeHeap\":\"" + formatBytes(ESP.getFreeHeap()) +
                "\", \"ChipSize\":\"" +  formatBytes(ESP.getFlashChipSize()) + "\", \"ChipSpeed\":\"" + ESP.getFlashChipSpeed() / 1000000 +
                "\", \"ChipMode\":\"" + flashChipMode[ESP.getFlashChipMode()] + "\", \"IdeVersion\":\"" + ARDUINO +
                "\", \"CoreVersion\":\"" + ESP.getCoreVersion() + "\", \"SdkVersion\":\"" + ESP.getSdkVersion() + "\"}";
  server.send(200, "application/json", temp);     // Json als Objekt
}

String runtime() {
  static uint8_t rolloverCounter = 0;
  static uint32_t letzteMillis = 0;
  auto aktuelleMillis = millis();
  if (aktuelleMillis < letzteMillis) {       // prüft Millis Überlauf
    rolloverCounter++;
  }
  letzteMillis = aktuelleMillis;
  auto sek = (0xFFFFFFFF / 1000 ) * rolloverCounter + (aktuelleMillis / 1000);
  char buf[20];
  snprintf(buf, sizeof(buf), "%d Tag%s %02d:%02d:%02d", sek / 86400, sek < 86400 || sek >= 172800 ? "e" : "", sek / 3600 % 24, sek / 60 % 60, sek % 60);
  return buf;
}
