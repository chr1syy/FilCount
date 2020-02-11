// Libraries

#include <SPI.h>
#include <Wire.h>
#include <FS.h>           // Include the SPIFFS library
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <u8g2_fonts.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <RotaryEncoder.h>
#include <ESP8266WiFi.h>
#include "languages.h"

#define VERSION "0.2 PRE ALPHA TEST"

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

// Display states
#define DISPLAY_INIT 0
#define DISPLAY_COUNTING 1
#define DISPLAY_MENU 2
#define DISPLAY_CONFIG 3

// Fonts
#define TEXT_FONT       u8g2_font_unifont_tf  // 10 pixel height
#define MENU_ITEM_FONT  u8g2_font_t0_11_te    // 8 pixel height
#define TEXT_FONT_HEIGHT 10

// Filesystem
#define CONFIG_JSON  "/config.json"
#define SPOOLS_JSON  "/spools.json"
File myfile;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// Encoder
// CLK - D5
// DT - D6
// SW - D7
// + - 3.3
// GND - G
#define LEFT true
#define RIGHT false
RotaryEncoder myEnc(D5, D6, D7);
int16_t position = 1;  // Init screen resets position to 0, so change position is triggered and counter will be shown

// Variables
struct filamentSpools
  {
    int id;
    String name;
    int length;
  };
filamentSpools spools[NUMBER_OF_SPOOLS];
int spoolCnt = 0;

int oldPos = 0;
int newPos = 0;
int selected = 0;
boolean spoolDirection = LEFT;
boolean buttonDirection;
boolean isButtonPressed = false;
int displayStatus = DISPLAY_INIT;

boolean isCounting = true;
boolean isMenu = false;
boolean isConfig = false; 
long lastUpdateMillis = 0;

void showMenu();
void showCounting();
void saveSpools();

// posX     = X position in pixel (CENTER for centered output)
// posY     = Y position in pixel (BOTTOM LEFT)
// text     = Text to print
// invers   = Print text inverted (true/false)
// addLines = Char to add before and after text (eg. for menu titles)
void printC(int posX, int posY, char* text, boolean invers = false, char* addDeco = NULL) {
  char ptext[strlen(text)+2];
  int plen;
  int lineHeight = u8g2.getFontAscent()+2;    // +2 because umlauts :)

  int textY = posY;                           // (0,0) is bottom left
  int drawY = posY +1 - lineHeight;              // (0,0) is top left

  // do we have to add something?
  if(addDeco != NULL) {
    strcpy(ptext, addDeco);                     // put one in front
    strcpy(ptext + 1, text);                    // now the text
    strcpy(ptext + strlen(text) + 1, addDeco);  // and one after
  } else {
    strcpy(ptext, text);                        // or ... just use the text
  }

  // ok, now the length is fixed
  plen = u8g2.getUTF8Width(ptext);

  // do we have to change the position for x?
  if(posX == CENTER) {
    posX = (int)((SCREEN_WIDTH - (plen + u8g2.getUTF8Width("A")) )/2); // Add a char the size of an "A" because reasons o_O
  }

  // ok, position is fixed, too
  // can't get u8g2 to print readable inversed text, so I make my own
  if(invers) {
    u8g2.setFontMode(1);
    display.fillRect(posX, drawY, plen + 1 , lineHeight + 2, WHITE);
    u8g2.setForegroundColor(BLACK);
  } else {
    u8g2.setFontMode(0);
    u8g2.setForegroundColor(WHITE);
  }

  // and now we print (finally!)
  u8g2.setCursor(posX, textY);
  u8g2.println(ptext); 
}

// Interrupt handler
void ICACHE_RAM_ATTR handleKey() {
  //myEnc.readPushButton();  
  isButtonPressed = true;
}

void ICACHE_RAM_ATTR handleRotation() {
  myEnc.readAB();
}

boolean getDirection() {
  static int oldDPos;
  int newDPos;
  boolean dir;
  
  newDPos = myEnc.getPosition();
  dir = newDPos > oldDPos ? LEFT : RIGHT;
  oldDPos = newDPos;
  return dir;
}

void showMenu(char state[20]){
  char* items[] = { MSG_MENU_ITEM_1, MSG_MENU_ITEM_2, MSG_MENU_ITEM_3, MSG_MENU_ITEM_4 };
  int items_size = sizeof(items) / sizeof(items[0]);
  
  int lineHeight;
  int nextLineAt = 0;
  int counter = 0;
  boolean buttonDirection;
  static int item_selected = 0;

  displayStatus = DISPLAY_MENU;
  buttonDirection = getDirection();  

  if(strcmp(state, "item_switched") == 0) {
    if(buttonDirection == LEFT && item_selected > 0) item_selected--;
    if(buttonDirection == RIGHT && item_selected < (items_size - 1)) item_selected++;
  }

  if(strcmp(state, "item_selected") == 0) {
    if(item_selected == 0) {
      displayStatus = DISPLAY_COUNTING;
      showCounting();
      return;
    }
  }
  
  if(strcmp(state, "init") == 0) {} // placeholder
  
  display.clearDisplay();
  u8g2.setFont(TEXT_FONT);
  lineHeight = u8g2.getFontAscent()+4;    // +4 because umlauts and some air:)  
  printC(CENTER, lineHeight, MSG_MENU_TITLE, false, "-");
  display.drawLine(0, lineHeight+2, SCREEN_WIDTH, lineHeight+2, WHITE);

  nextLineAt = lineHeight * 2;    
  u8g2.setFont(MENU_ITEM_FONT);
  lineHeight = u8g2.getFontAscent()+2;

  while(nextLineAt < SCREEN_HEIGHT && counter < items_size) {  // as long as we see something and are not out of items to display     
    printC(CENTER, nextLineAt, items[counter], (counter == item_selected ? true : false) );
    nextLineAt += lineHeight;
    counter++;
  }
  display.display();
}

void loadSpools() {
  boolean failed = false;
  const char* tmpName;
  int tmpId, i;
  
  myfile = SPIFFS.open(SPOOLS_JSON, "r");
  if (myfile) {
    size_t size = myfile.size();
    StaticJsonDocument<2048> doc;  // has to be more precise

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, myfile);
    if (error) {
      Serial.println(F(PRG_FILE_FAILED));
      Serial.print(F(PRG_FILE_CODE));
      Serial.println(error.c_str());
      failed = true;
    } else {
      JsonArray jspools = doc["spools"];
      for(i=0; i<NUMBER_OF_SPOOLS; i++){
        tmpId = jspools[i]["id"];   // JsonArray is overloaded in mysterious ways ;)
        if(tmpId != 0) {
          spools[i].id     = tmpId;
          tmpName          = jspools[i]["name"];
          spools[i].name   = String(tmpName);
          spools[i].length = jspools[i]["length"];
        } else {
          spoolCnt = i;
          Serial.print(i); Serial.println(PRG_SPOOLS_LOADED);
          break;
        }
      }
    }
    myfile.close();    
  } else {
    Serial.println(F(PRG_NO_SPOOLS));
    Serial.println(F(PRG_CREATE_SPOOLS));
    failed = true;
  }

  if(failed) {
    spools[0] = {1, DUMMY_SPOOL, 0};
    spoolCnt = 1;
    saveSpools();
  }    
  /*
  spools[0] = {1, "PLA Rot", 22};
  spools[1] = {2, "PETG Blau", 0};
  spools[2] = {3, "TPU Lila", 50};
  spoolCnt = 3;
  */
}

void saveSpools() {
  /* spools[0] = {1, "PLA Rot", 22};
  spools[1] = {2, "PETG Blau", 0};
  spools[2] = {3, "TPU Lila", 50};
  spoolCnt = 3; */
    
  const size_t bufferSize = JSON_ARRAY_SIZE(spoolCnt) + JSON_OBJECT_SIZE(1) + spoolCnt*JSON_OBJECT_SIZE(3) + (30+(spoolCnt*30));
  DynamicJsonDocument root(bufferSize);
  
  JsonArray block = root.createNestedArray("spools");
  for(int i = 0; i < spoolCnt; i++) {
    if(spools[i].id == NULL) break;
    JsonObject step = block.createNestedObject();
    step["id"] = spools[i].id;
    step["name"] = spools[i].name;
    step["length"] = spools[i].length;
  }
  //SPIFFS.remove(SPOOLS_JSON);
  myfile = SPIFFS.open(SPOOLS_JSON, "w");
  serializeJson(root, myfile);
  myfile.close();
  //Serial.println();
  //serializeJsonPretty(root, Serial);
  //Serial.println();
}

void showStart(){
  // TODO: a nice an breathtaking startup sequence with gfx, animation and totally important information
  // RealityCheck: for now (and probably forever) some boring infos
  display.clearDisplay();
  u8g2.setForegroundColor(WHITE);
  u8g2.setFont(TEXT_FONT); // 10 pixel font
  u8g2.setCursor(0, 11);
  u8g2.println(MSG_TITLE);
  u8g2.setFont(MENU_ITEM_FONT);
  u8g2.println(MSG_INIT);
  display.display();
  delay(2000);
}

void showCounting() {
  selected = 0;
  display.clearDisplay();
  u8g2.setCursor(0,11);
  u8g2.print(spools[selected].name);  
  u8g2.setCursor(55,40);
  
  // Werte zählen und anzeigen
  newPos = myEnc.getPosition();
  oldPos = newPos;
  u8g2.print(oldPos);
  display.display();
}

boolean InitalizeFileSystem() {
  bool initok = false;
  initok = SPIFFS.begin();
  if (!(initok)) // Format SPIFS, if not formatted. - Try 1
  {
    Serial.println(F(PRG_SPIFFS_FORMAT));
    SPIFFS.format();
    initok = SPIFFS.begin();
  }
  if (!(initok)) // Format SPIFS. - Try 2
  {
    SPIFFS.format();
    initok = SPIFFS.begin();
  }
  if (initok) { Serial.println(F(PRG_SPIFFS_OK)); } else { Serial.println(F(PRG_SPIFFS_NOT_OK)); }
  return initok;
}

void setup() { 
  Serial.begin(9600);
  Serial.println();
  Serial.print(F("Version: "));
  Serial.println(F(VERSION));
  Serial.print(F("Build: "));
  Serial.print(__TIME__);
  Serial.print(F("  "));
  Serial.println(__DATE__);
  Serial.println(__FILE__);   
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  u8g2.begin(display);
  SPI.begin();                      // Initialisiere SPI Kommunikation
  bool Result  = InitalizeFileSystem();   
  displayStatus = DISPLAY_INIT;
  //saveSpools();
  loadSpools();
 // saveSpools();
  showStart();

  // Interrupt auf Pin D7 für Button, D5 & D6 für Rotary -> funktioniert schlechter als DO_NOT_USE_INTERRUPTS
  myEnc.begin();
  attachInterrupt(digitalPinToInterrupt(D5), handleRotation, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D6), handleRotation, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D7), handleKey, RISING);
  myEnc.setPosition(0);
  
  displayStatus = DISPLAY_COUNTING;
}

void loop() {
  u8g2.setFont(TEXT_FONT);
  if (isButtonPressed && millis() - lastUpdateMillis > 50) {  
    isButtonPressed = false;
    lastUpdateMillis = millis();
  }  
  if(isButtonPressed) {
    isButtonPressed = false;
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
    delay(50);
  }

  if(position != myEnc.getPosition()) {
    position = myEnc.getPosition();
    switch(displayStatus){
      case DISPLAY_COUNTING:
            showCounting();
            break;
      case DISPLAY_MENU:
            showMenu("item_switched");
            break;
      default:
            break;
    }
  }
  
/*
    // Wenn auf den Knopf gedrückt wird
    if (isButtonPressed && millis() - lastUpdateMillis > 50) {
      isButtonPressed = false;
      lastUpdateMillis = millis();


      // Zählwert speichern und auf den alten Menüeintrag zurück springen
      spools[selected].length = oldPos;
      myEnc.setPosition(selected);

      isCounting = false;
      break;
    }
    */

// ab hier ... Chaos :D Working chaos, but still...
/*
  // Merker für spools[x]
  selected = oldPos;
  
  display.clearDisplay();
  u8g2.setCursor(0,11);
  u8g2.print(spools[selected].name);  
  u8g2.setCursor(55,40);

  newPos = myEnc.getPosition();
  
  // Einstiegs-Menü, Auswahl der Rolle
  // Unter ID 1 wollen wir nicht auswählen
  if (newPos < 0) {
    oldPos = 0;
    myEnc.setPosition(0);
  }
  // Gibt es den nächsten Datensatz überhaupt? -> Neue Rolle erstellen / Einstellungen als Menüpunkt hier einfügen? 
  else if (spools[newPos].id == 0){
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
    myEnc.setPosition(spools[selected].length);
 }

  
  u8g2.print(spools[selected].length);
  display.display();
*/
}
