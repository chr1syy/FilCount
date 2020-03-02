#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
//#include <ArduinoOTA.h> // OTA Upload via ArduinoIDE

#ifndef STASSID
#define STASSID "Your SSID"
#define STAPSK  "Your password"

#endif
#define WIFI_RETRIES 10

char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

String sketchName() {                             // Dateiname für den Admin Tab
  char file[sizeof(__FILE__)] = __FILE__;
  char * pos = strchr(file, '.'); *pos = '\0';
  return file;
}
