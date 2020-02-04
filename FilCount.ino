// Libraries

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <u8g2_fonts.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <RotaryEncoder.h>
#include <ESP8266WiFi.h>
#include "languages.h"


// OLED I²C Display
// SCL - D1
// SDA - D2
// VCC - 3.3
// GND - G
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// Encoder
// CLK - D5
// DT - D6
// SW - D7
// + - 3.3
// GND - G
RotaryEncoder myEnc(D5, D6, D7);

int oldPos = 0;
int newPos = 0;
boolean isButtonPressed = false;
long lastUpdateMillis = 0;

// Interrupt handler

void ICACHE_RAM_ATTR handleKey() {
  isButtonPressed = true;  
}

void ICACHE_RAM_ATTR handleRot() {
  myEnc.readAB();
}

// Variable

String names[10];
char pos[5];
long len[10];


void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  u8g2.begin(display);
  myEnc.begin();
  
  display.clearDisplay();
  //display.setTextSize(1);
  u8g2.setForegroundColor(WHITE);
  u8g2.setFont(u8g2_font_unifont_tf); // 10 pixel font
  u8g2.setCursor(0, 11);
  u8g2.println(MSG_TITLE);
  u8g2.println("Init");
  display.display();

  delay(1000);

  // Interrupt auf Pin D7 für Button, D5 & D6 für Rotary -> funktioniert schlechter als DO_NOT_USE_INTERRUPTS
  attachInterrupt(digitalPinToInterrupt(D5), handleRot, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D6), handleRot, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D7), handleKey, RISING);
  
  myEnc.setPosition(0);
}

void loop() {
  display.clearDisplay();
  //display.setTextSize(2);  
  u8g2.setCursor(30,20);
  newPos = myEnc.getPosition();
  
  if (newPos != oldPos){
    oldPos = newPos;
  }

  if (isButtonPressed && millis() - lastUpdateMillis > 50) {  // debounce Rotary
    isButtonPressed = false;
    lastUpdateMillis = millis();
    // Reset the counter
    myEnc.setPosition(0);
  }
  itoa(oldPos, pos, 10);
  u8g2.print(oldPos);
  display.display();

}
