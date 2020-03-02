# FilCount
FilamentCounter

# Arduino Settings:

Boardmanager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json 

LOLIN(Wemos) D1 R2 Mini 

Upload Speed: 921600 

CPU Speed: 80 MHz 

Flash Size: 4MB,  1019 kB OTA 

# Libraries

https://github.com/enjoyneering/RotaryEncoder

https://github.com/olikraus/U8g2_for_Adafruit_GFX

# AddOns

<b>arduino-esp8266fs-plugin</b><br>
https://github.com/esp8266/arduino-esp8266fs-plugin/releases/tag/0.5.0<br>
Note: Serial Monitor has to be turned off to upload!<br>
WARNING: ALL existing data will be overwritten!!<br>
<br>
For initial upload copy all files from /webserver to /data<br>
<b>OR</b> use the build-in Webserver to upload the files.


# SPIFFS

To upload/edit files in SPIFFS instead of the <b>arduino-esp8266fs-plugin</b> you can use the build-in webserver (thanks to https://fipsok.de/Esp8266-Webserver/esp8266 )
Just upload <b>spiffs.html</b> and <b>spiffs.css</b> first.

# FunLevel
mindestens 2
