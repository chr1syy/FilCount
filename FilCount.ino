// Libraries
// Encoder funktioniert nur ohne Interrupt -> eventuell andere Library / ICACHE_RAM_ATTR in der Library einfügen

#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>


// OLED I²C Display
// SCL - D1
// SDA - D2
// VCC - 3.3
// GND - G

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Encoder
// CLK - D5
// DT - D6
// SW - D7
// + - 3.3
// GND - wohin wohl
Encoder myEnc(D5, D6);

int oldPos = -999;
boolean isButtonPressed = false;
long lastUpdateMillis = 0;

// Interrupt handler

void ICACHE_RAM_ATTR handleKey() {
  isButtonPressed = true;  
}

void setup() {
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Display-Test");
  delay(100);
  display.display();

  // Interrupt auf Pin D7 für Button

  attachInterrupt(digitalPinToInterrupt(D7), handleKey, RISING);
  
  myEnc.write(0);
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  int newPos = myEnc.read();
  if (newPos != oldPos){
    oldPos = newPos;
  }

  if (isButtonPressed && millis() - lastUpdateMillis > 50) {
    isButtonPressed = false;
    lastUpdateMillis = millis();
    // Reset the counter
    myEnc.write(0);
  }
  
  display.println(oldPos);
  display.display();

}
