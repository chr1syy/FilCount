// Libraries

//#include <SPI.h>
#include <Wire.h>
#include <FS.h>           // Include the SPIFFS library
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <u8g2_fonts.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <RotaryEncoder.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> // OTA Upload via ArduinoIDE

#include "languages.h"
#include "server.h"
#include "logo.h"

#define VERSION "V0.3 BETA"

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
#define DISPLAY_SPOOLSELECT 4
#define DISPLAY_SELECTION 5
#define DISPLAY_WIFI 6

#define ITEM_SELECTED  1
#define ITEM_SWITCHED  2
#define INIT           3
#define SPOOL_SELECTED 4
#define BACK 1
#define SELECT 2
#define CANCEL 3

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
    const char* ident;
    const char* name;
    int length;
  };
filamentSpools spools[NUMBER_OF_SPOOLS];
int spoolCnt = 0;

int oldPos = 0;
int newPos = 0;
int selected = 0;
int lineHeightText = 0;
int lineHeightMenu = 0;
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
void selectSpool(int);
void showSelection(int);
void showWifi();
void StartupWiFi();

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
  if(posX == LEFT) {
    posX = 1;
  }
  if(posX == RIGHT) {
    posX = (int)(SCREEN_WIDTH - (plen + u8g2.getUTF8Width("A")) );
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
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 400)
  {
    isButtonPressed = true;
  }
  last_interrupt_time = interrupt_time;
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

void showMenu(int state){
  char* items[] = { MSG_MENU_ITEM_1, MSG_MENU_ITEM_2, MSG_MENU_ITEM_3, MSG_MENU_ITEM_4 };
  int items_size = sizeof(items) / sizeof(items[0]);
  
  int lineHeight;
  int nextLineAt = 0;
  int counter = 0;
  boolean buttonDirection;
  static int item_selected = 0;
  static int buff = 0;

  displayStatus = DISPLAY_MENU;
  buttonDirection = getDirection();  

  if(state == ITEM_SWITCHED) {
    if(buff == 0) { // only every 2nd step
      buff = 1;
      return;
    }
    buff = 0;    
    if(buttonDirection == LEFT && item_selected > 0) item_selected--;
    if(buttonDirection == RIGHT && item_selected < (items_size - 1)) item_selected++;
  }

  if(state == ITEM_SELECTED) {
    switch(item_selected) {
      case 0:
          myEnc.setPosition(oldPos);  // continue where we left
          showCounting();
          return;
          break;
      case 1:
          selectSpool(INIT);
          return;
          break;
      case 2:
          showWifi();
          return;
          break;          
      default:
          break;
    }
  }
  
  if(state == INIT) {} // placeholder
  
  display.clearDisplay();
  u8g2.setFont(TEXT_FONT);
  lineHeight = lineHeightText;
  printC(CENTER, lineHeight, MSG_MENU_TITLE, false, "-");
  display.drawLine(0, lineHeight+2, SCREEN_WIDTH, lineHeight+2, WHITE);

  nextLineAt = lineHeight * 2;    
  u8g2.setFont(MENU_ITEM_FONT);
  lineHeight = lineHeightMenu;

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
         // tmpName          = jspools[i]["ident"];
          spools[i].ident  = jspools[i]["ident"];
          //tmpName          = jspools[i]["name"];
          spools[i].name   = jspools[i]["name"];
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
    spools[0] = {1, "SP-01", DUMMY_SPOOL, 0};
    spoolCnt = 1;
    saveSpools();
  }    
  /*
  spools[0] = {1, "SP-01", "PLA Rot", 22};
  spools[1] = {2, "SP-02", "PETG Blau", 0};
  spools[2] = {3, "SP-03", "TPU Lila", 50};
  spoolCnt = 3;
  */
}

void saveSpools() {
  /* spools[0] = {1, "SP-01", "PLA Rot", 22};
  spools[1] = {2, "SP-02", "PETG Blau", 0};
  spools[2] = {3, "SP-03", "TPU Lila", 50};
  spoolCnt = 3; */
    
  const size_t bufferSize = JSON_ARRAY_SIZE(spoolCnt) + JSON_OBJECT_SIZE(1) + spoolCnt*JSON_OBJECT_SIZE(3) + (30+(spoolCnt*30));
  DynamicJsonDocument root(bufferSize);
  
  JsonArray block = root.createNestedArray("spools");
  for(int i = 0; i < spoolCnt; i++) {
    if(spools[i].id == NULL) break;
    JsonObject step = block.createNestedObject();
    step["id"] = spools[i].id;
    step["ident"] = spools[i].ident;
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

void selectSpool(int state){
  displayStatus = DISPLAY_SPOOLSELECT;
  int lineHeight, nextLineAt;
  boolean buttonDirection;
  boolean show_back = false;
  boolean show_next = true;
  int counter = 0;
  static int item_selected = 0;
  static int buff = 0;

  buttonDirection = getDirection();  

  if(state == SPOOL_SELECTED) {
    selected = item_selected;
    oldPos = spools[selected].length;
    myEnc.setPosition(oldPos);  // continue at spool count
    showCounting();
    return;
  }

  if(state == ITEM_SWITCHED) {
    if(buff == 0) { // only every 2nd step
      buff = 1;
      return;
    }
    buff = 0;
    if(buttonDirection == LEFT && item_selected > 0) item_selected--;
    if(buttonDirection == RIGHT && item_selected < (spoolCnt - 1)) item_selected++;
  }

  if(item_selected <= 0) {
    show_back = false;
    show_next = true;
  } else if(item_selected >= spoolCnt-1){
    show_back = true;
    show_next = false;
  } else {
    show_back = true;
    show_next = true;
  }

  display.clearDisplay();
  u8g2.setFont(MENU_ITEM_FONT);
  lineHeight = lineHeightMenu; 
  printC(CENTER, lineHeight, MSG_SPOOL_TITLE, false, "-");
  display.drawLine(0, lineHeight+2, SCREEN_WIDTH, lineHeight+2, WHITE);

  // Show spool information
  nextLineAt = lineHeight + lineHeight;
  u8g2.setCursor(0, nextLineAt);
  u8g2.println(spools[item_selected].ident);
  u8g2.setFont(TEXT_FONT); // 10 pixel font
  nextLineAt = nextLineAt + lineHeightText;
  u8g2.setCursor(0, nextLineAt);
  u8g2.println(spools[item_selected].name);
  u8g2.setFont(MENU_ITEM_FONT);
  nextLineAt = nextLineAt + lineHeightMenu;
  u8g2.setCursor(0, nextLineAt);
  u8g2.println(spools[item_selected].length);
  nextLineAt = SCREEN_HEIGHT;
  if(show_back) {
    u8g2.setCursor(0, nextLineAt);
    u8g2.print(" <<");  
  }
  if(show_next) {
    u8g2.setCursor((SCREEN_WIDTH - u8g2.getUTF8Width(">> ")), nextLineAt);
    u8g2.print(">> ");  
  }
  display.display();
  
  Serial.println("Look at these spools!");
  return;
}

void showSelection(int state){
  displayStatus = DISPLAY_SELECTION;
  char* items[] = { MSG_SPOOL_BACK, MSG_SPOOL_SELECT, MSG_SPOOL_CANCEL };
  int items_size = 3;
  int lineHeight, nextLineAt;
  boolean buttonDirection;
  int counter = 0;
  static int item_selected = 0;
  static int buff = 0;

  buttonDirection = getDirection();  

  if(state == ITEM_SWITCHED) {
    if(buff == 0) { // only every 2nd step
      buff = 1;
      return;
    }
    buff = 0;    
    if(buttonDirection == LEFT && item_selected > 0) item_selected--;
    if(buttonDirection == RIGHT && item_selected < (items_size - 1)) item_selected++;
  }

  if(state == ITEM_SELECTED) {
    switch(item_selected) {
      case 0:
          showMenu(ITEM_SELECTED);
          return;
          break;
      case 1:
          selectSpool(SPOOL_SELECTED);
          return;
          break;
      case 2:
          showMenu(INIT);
          return;
          break;
      default:
          break;
    }
  }
      
  u8g2.setFont(MENU_ITEM_FONT);
  lineHeight = lineHeightMenu;
  nextLineAt = lineHeight * 2 + 2 ;  // Can't remember why *2 but it works
  
  display.fillRect(20, 10, 88, 44, BLACK);
  display.drawRect(20, 10, 88, 44, WHITE);

  while(counter < items_size) {
    printC(CENTER, nextLineAt, items[counter], (counter == item_selected ? true : false) );
    nextLineAt += lineHeight;
    counter++;
  }  
  display.display();
}
void Ip2chr(char Ip[]) {
  IPAddress ip = WiFi.localIP();

  //ip.toString().toCharArray(Ip, 16);  // this looks not good at all
  sprintf(Ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void showWifi(){
  char Ip[] = "xxx.xxx.xxx.xxx";
  
  displayStatus = DISPLAY_WIFI;
  int lineHeight;
  int nextLineAt = 0;

  display.clearDisplay();
  u8g2.setFont(TEXT_FONT);
  lineHeight = lineHeightText;
  printC(CENTER, lineHeight, MSG_WIFI_TITLE, false, "-");
  display.drawLine(0, lineHeight+2, SCREEN_WIDTH, lineHeight+2, WHITE);

  nextLineAt = lineHeight * 2 + 5;

  Ip2chr(Ip);
  if(strcmp(Ip, "0.0.0.0") != 0 ){
     display.setTextWrap(true);
     printC(CENTER, nextLineAt, ssid, false);
     nextLineAt = nextLineAt + lineHeightText + 6;
     printC(CENTER, nextLineAt, Ip, false);
  } else {
    printC(CENTER, nextLineAt, MSG_NO_WIFI, false);
  }

  display.display();
}

void showStart(){
  // TODO: a nice an breathtaking startup sequence with gfx, animation and totally important information
  // RealityCheck: for now (and probably forever) some boring infos
  display.clearDisplay();
  u8g2.setForegroundColor(WHITE);
  u8g2.setFont(TEXT_FONT); // 10 pixel font
  lineHeightText = u8g2.getFontAscent()+3;    // +4 because umlauts and some air:)
  u8g2.setCursor(0, 11);
  u8g2.println(F(MSG_TITLE));
  u8g2.setFont(MENU_ITEM_FONT);
  lineHeightMenu = u8g2.getFontAscent()+3;    // +4 because umlauts and some air:)
  u8g2.println(F(MSG_INIT));
  display.display();
  StartupWiFi();
  delay(2000);
}

void showCounting() {
  //selected = 0;

  displayStatus = DISPLAY_COUNTING;
  display.clearDisplay();
  u8g2.setFont(TEXT_FONT);
  u8g2.setCursor(0,11);
  u8g2.print(spools[selected].name);  
  u8g2.setCursor(55,40);
  
  // Werte zählen und anzeigen
  newPos = myEnc.getPosition();
  oldPos = newPos;
  spools[selected].length = newPos;
  u8g2.print(spools[selected].length);
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

void StartupWiFi(){
  char Ip[] = "xxx.xxx.xxx.xxx";
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  u8g2.println(F(MSG_SEARCH_WIFI));
  display.display();
  // Wait for connection
  int intRetry = 0;
  while ((WiFi.status() != WL_CONNECTED) && (intRetry < WIFI_RETRIES)){
    Serial.print(".");
    u8g2.print(".");
    display.display();
    intRetry++;
    delay(500);
  }

  Serial.println("");
  Ip2chr(Ip);
  if(strcmp(Ip, "0.0.0.0") != 0 ){
    Serial.print(F("Connected to "));
    Serial.println(ssid);
    Serial.print(F("IP address: "));    
    Serial.println(Ip);
    u8g2.println(F(MSG_FOUND_WIFI));
    display.display();    
  } else {
    Serial.println(F(MSG_NO_WIFI));
    u8g2.println(F(MSG_NO_WIFI));
    display.display();    
  }
}

void setup() { 
  Serial.begin(9600);
  Serial.println();
  Serial.print(F("Version: "));
  Serial.println(F(VERSION));
  Serial.print(F("Build: "));
  Serial.print(F(__TIME__));
  Serial.print(F("  "));
  Serial.println(F(__DATE__));
  Serial.println(F(__FILE__));   

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  u8g2.begin(display);
  display.clearDisplay();
  display.drawBitmap(0, 0, logo, 128, 64, 1);
  display.display();  
  delay(1000);
  
  bool Result  = InitalizeFileSystem();
  
  // Interrupt auf Pin D7 für Button, D5 & D6 für Rotary -> funktioniert schlechter als DO_NOT_USE_INTERRUPTS
  myEnc.begin();
  attachInterrupt(digitalPinToInterrupt(D5), handleRotation, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D6), handleRotation, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D7), handleKey, RISING);  
  
  displayStatus = DISPLAY_INIT;
  
  loadSpools();
  myEnc.setPosition(spools[selected].length);
  Serial.print(F("Selected: "));
  Serial.println(spools[selected].name);
  Serial.print(F("Length: "));
  Serial.println(spools[selected].length);  
 // saveSpools();
  showStart();
  startOTA();
  
  server.begin();
  spiffs();
  admin();

//  if (MDNS.begin("esp8266")) {
//    Serial.println(F("MDNS responder started"));
//  }
  
  displayStatus = DISPLAY_COUNTING;
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  u8g2.setFont(TEXT_FONT);
  runtime();
  
  if(isButtonPressed) {
    isButtonPressed = false;
    switch(displayStatus) {
      case DISPLAY_INIT:
            // do nothing
            break;
      case DISPLAY_COUNTING:
            saveSpools();
            showMenu(INIT);
            break;
      case DISPLAY_MENU:
            showMenu(ITEM_SELECTED);
            break;
      case DISPLAY_WIFI:
            showMenu(INIT);
            break;            
      case DISPLAY_CONFIG:
            //showConfig(ITEM_SELECTED);
            break;
      case DISPLAY_SPOOLSELECT:
            showSelection(INIT);
            break;
      case DISPLAY_SELECTION:
            showSelection(ITEM_SELECTED);
            break;            
      default:
            break;
    }
    //delay(50);
  }

  if(position != myEnc.getPosition()) {
    position = myEnc.getPosition();
    switch(displayStatus){
      case DISPLAY_COUNTING:
            showCounting();
            break;
      case DISPLAY_MENU:
            showMenu(ITEM_SWITCHED);
            break;
      case DISPLAY_SELECTION:
            showSelection(ITEM_SWITCHED);
            break;
      case DISPLAY_SPOOLSELECT:
            selectSpool(ITEM_SWITCHED);
            break;
      default:
            break;
    }
  }
}
