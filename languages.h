#define LANG_DE
#ifdef LANG_DE
  #define MSG_TITLE         "Filament-Zähler"
  #define MSG_INIT          "Initialisiere"
  #define MSG_MENU_TITLE    "Menü"
  #define MSG_MENU_ITEM_1   "zurück..."  
  #define MSG_MENU_ITEM_2   "Wähle Spule"
  #define MSG_MENU_ITEM_3   "WiFi"
  #define MSG_MENU_ITEM_4   "Einstellungen"
  #define DUMMY_SPOOL       "Startspule"
  #define PRG_FILE_FAILED   "Konnte Datei nicht lesen, benutze Standardkonfiguration"
  #define PRG_FILE_CODE     "deserializeJson() gescheitert mit Code "
  #define PRG_NO_SPOOLS     "spools.json nicht gefunden"
  #define PRG_CREATE_SPOOLS "Erstelle spools.json ..."
  #define PRG_SPIFFS_FORMAT "SPIFFS Dateisystem formatiert."
  #define PRG_SPIFFS_OK     "SPIFFS ist OK"
  #define PRG_SPIFFS_NOT_OK "SPIFFS ist NICHT OK"
  #define PRG_SPOOLS_LOADED " Spulen geladen."
#endif

//#define LANG_EN
#ifdef LANG_EN
  #define MSG_TITLE         "Filamant-Counter"
  #define MSG_INIT          "Init"
  #define MSG_MENU_TITLE    "Menu"
  #define MSG_MENU_ITEM_1   "back..."
  #define MSG_MENU_ITEM_2   "Select spool"
  #define MSG_MENU_ITEM_3   "WiFi"
  #define MSG_MENU_ITEM_4   "Settings"
  #define DUMMY_SPOOL       "First spool"
  #define PRG_FILE_FAILED   "Failed to read file, using default configuration"
  #define PRG_FILE_CODE     "deserializeJson() failed with code "
  #define PRG_NO_SPOOLS     "No spools.json found"
  #define PRG_CREATE_SPOOLS "Creating spools.json ..."
  #define PRG_SPIFFS_FORMAT "SPIFFS filesystem formatted."
  #define PRG_SPIFFS_OK     "SPIFFS is OK"
  #define PRG_SPIFFS_NOT_OK "SPIFFS is NOT OK"
  #define PRG_SPOOLS_LOADED " spools loaded."
#endif
