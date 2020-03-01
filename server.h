#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h> // OTA Upload via ArduinoIDE

#ifndef STASSID
#define STASSID "Your SSID"
#define STAPSK  "Your password"
#endif

char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);
