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

#define NUMBER_OF_SPOOLS 50

// OLED I²C Display
// SCL - D1
// SDA - D2
// VCC - 3.3
// GND - G
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    0
#define CENTER -1

#define DISPLAY_INIT 0
#define DISPLAY_COUNTING 1
#define DISPLAY_MENU 2
#define DISPLAY_CONFIG 3

#define TEXT_FONT u8g2_font_unifont_tf
#define TEXT_FONT_HEIGTH 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// Encoder
// CLK - D5
// DT - D6
// SW - D7
// + - 3.3
// GND - G
RotaryEncoder myEnc(D5, D6, D7);

// Variables
struct filamentRolls
  {
    int id;
    String name;
    int length;
  };
filamentRolls rolls[NUMBER_OF_SPOOLS];

int oldPos = 0;
int newPos = 0;
int selected = 0;
boolean spoolDirection = true; // true = left, false = right;
boolean buttonDirection = true;

boolean isButtonPressed = false;
boolean posChanged = true;  // True for initial display of counter
int displayStatus = DISPLAY_INIT;

boolean isCounting = true;
boolean isMenu = false;
boolean isConfig = false; 
long lastUpdateMillis = 0;

void showMenu();

// pos      = X position in pixel (CENTER for centered output)
// line     = Y position in lines
// text     = Text to print
// invers   = Print text inverted (true/false)
// addLines = Char to add before and after text (eg. for menu titles)
void printC(int pos, int line, char* text, boolean invers = false, char* addDeco = NULL) {
  char ptext[strlen(text)+2];
  int plen;
  int lineHeight = u8g2.getFontAscent()+2;

  int textY = (lineHeight + 1) * line;        // (0,0) is bottom left
  int drawY = textY - lineHeight;             // (0,0) is top left

  // do we have to add something?
  if(addDeco != NULL) {
    strcpy(ptext, addDeco);                     // put one in front
    strcpy(ptext + 1, text);                    // now the text
    strcpy(ptext + strlen(text) + 1, addDeco);  // and one after
  } else {
    strcpy(ptext, text);                      // or ... just use the text
  }

  // ok, now the length is fixed
  plen = u8g2.getUTF8Width(ptext);

  // do we have to change the position for x?
  if(pos == CENTER) {
    pos = (int)((SCREEN_WIDTH - (plen + u8g2.getUTF8Width("A")) )/2); // Add a char because reasons o_O
  }

  // ok, position is fixed, too
  // can't get u8g2 to print inversed text, so I make my own
  if(invers) {
    display.fillRect(pos, drawY, plen + 1 , lineHeight + 1, WHITE);
    u8g2.setForegroundColor(BLACK);
  } else {
    u8g2.setForegroundColor(WHITE);
  }
  
  // and now we print
  u8g2.setFont(u8g2_font_unifont_tf); // 10 pixel font / TODO: make variable
  u8g2.setCursor(pos, textY);
  u8g2.println(ptext); 
}

// Interrupt handler
void ICACHE_RAM_ATTR handleKey() {
  isButtonPressed = true;
}

void ICACHE_RAM_ATTR handleRotLeft() {
  myEnc.readAB();
  buttonDirection = true;
  posChanged = true;
}

void ICACHE_RAM_ATTR handleRotRight() {
  myEnc.readAB();
  buttonDirection = false;
  posChanged = true;
}

void showMenu(char state[20]){
  // Show Startup
  //detachInterrupt(digitalPinToInterrupt(D7));
  display.clearDisplay();
  u8g2.setFontMode(1);
  printC(CENTER, 1, MSG_MENU_TITLE, true, "-");
  printC(CENTER, 2, MSG_MENU_TITLE, true, "-");
  /*u8g2.setForegroundColor(WHITE);
  u8g2.setFont(TEXT_FONT); // 10 pixel font
  u8g2.setCursor(0, 11);
  u8g2.println(MSG_MENU_TITLE); */
  
  display.display();
  delay(3000);  
  //attachInterrupt(digitalPinToInterrupt(D7), handleKey, RISING);
}

void getRolls() {
  // Get the rolls from whoknowswhere
  // Beispiele für Rollen 
  rolls[0].id = 1;
  rolls[0].name = "PLA Rot";
  rolls[0].length = 22;
  rolls[1] = {2, "PETG Blau", 0};
  rolls[2] = {3, "TPU Lila", 50};
}

void showStart(){
  // TODO: a nice an breathtaking startup sequence with gfx, animation and totally important information
  // RealityCheck: for now (and probably forever) some boring infos
  display.clearDisplay();
  u8g2.setForegroundColor(WHITE);
  u8g2.setFont(u8g2_font_unifont_tf); // 10 pixel font
  u8g2.setCursor(0, 11);
  u8g2.println(MSG_TITLE);
  u8g2.println(MSG_INIT);
  display.display();
  delay(3000);
}
void setup() {     
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  u8g2.begin(display);
  displayStatus = DISPLAY_INIT;
  myEnc.begin();
  getRolls();
  showStart();

  // Interrupt auf Pin D7 für Button, D5 & D6 für Rotary -> funktioniert schlechter als DO_NOT_USE_INTERRUPTS
  attachInterrupt(digitalPinToInterrupt(D5), handleRotLeft, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D6), handleRotRight, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D7), handleKey, RISING);
  
  myEnc.setPosition(0);
  displayStatus = DISPLAY_COUNTING;
}

void loop() {
  if(isButtonPressed) {
    u8g2.println(displayStatus);
    switch(displayStatus) {
      case DISPLAY_INIT:
            // do nothing
            break;
      case DISPLAY_COUNTING:
            showMenu("init");
            break;
      case DISPLAY_MENU:
            showMenu("item_selected");
            break;
      case DISPLAY_CONFIG:
            //showConfig("item_selected");
            break;
      default:
            break;
    }    
  } else {
  // Falls im letzten Loop-Durchlauf der Zählmodus aktiviert wurde, springe in while-loop
  if(posChanged && displayStatus == DISPLAY_COUNTING){
    selected = 0;
    display.clearDisplay();
    u8g2.setCursor(0,11);
    u8g2.print(rolls[selected].name);  
    u8g2.setCursor(55,40);

    // Werte zählen und anzeigen
    newPos = myEnc.getPosition();
    oldPos = newPos;

    u8g2.print(oldPos);
    display.display();
    posChanged = false;
/*
    // Wenn auf den Knopf gedrückt wird
    if (isButtonPressed && millis() - lastUpdateMillis > 50) {
      isButtonPressed = false;
      lastUpdateMillis = millis();


      // Zählwert speichern und auf den alten Menüeintrag zurück springen
      rolls[selected].length = oldPos;
      myEnc.setPosition(selected);

      isCounting = false;
      break;
    }
    */
  }
  } 

// ab hier ... Chaos :D Working chaos, but still...
/*
  // Merker für rolls[x]
  selected = oldPos;
  
  display.clearDisplay();
  u8g2.setCursor(0,11);
  u8g2.print(rolls[selected].name);  
  u8g2.setCursor(55,40);

  newPos = myEnc.getPosition();
  
  // Einstiegs-Menü, Auswahl der Rolle
  // Unter ID 1 wollen wir nicht auswählen
  if (newPos < 0) {
    oldPos = 0;
    myEnc.setPosition(0);
  }
  // Gibt es den nächsten Datensatz überhaupt? -> Neue Rolle erstellen / Einstellungen als Menüpunkt hier einfügen? 
  else if (rolls[newPos].id == 0){
    myEnc.setPosition(oldPos);
  }
  // Ansonsten weiter scrollen
  else {
    oldPos = newPos;
  }

  // Wenn der Knopf gedrückt wird die aktuelle Rolle auswählen
  if (isButtonPressed && millis() - lastUpdateMillis > 50) {  
    isButtonPressed = false;
    lastUpdateMillis = millis();

    isCounting = true;
    // Encoder auf Zählwert stellen, ab dort dann weiter zählen
    myEnc.setPosition(rolls[selected].length);
 }

  
  u8g2.print(rolls[selected].length);
  display.display();
*/
}
