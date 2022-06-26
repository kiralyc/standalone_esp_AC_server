# standalone_esp_AC_server

This is a basic project for using an esp32 with a ky-005 IR modul for remote control an old dumb AC. 

I had integrated a XIAOMI Mijia LYWSD03MMC thermometer with a custom pvvx custom firmware based on this [repo](https://github.com/pvvx/ATC_MiThermometer) (with ATC 1441 advertise format) and also integrated a duckdns dynamic dns library for a better usability. 
The whole project is based on some public examples ([Websocket](https://randomnerdtutorials.com/esp32-web-server-websocket-sliders), [heatpumpir](https://github.com/ToniA/arduino-heatpumpir), [EasyDDNS](https://github.com/ayushsharma82/EasyDDNS)) via Arduino IDE. 
I have used M5Stack Atom Lite esp board because it is fully packed and it has everything what I needed. Unfortunately the integrated IR led (PIN 12) in M5 Atom Lite has a very limited usability range (only 60cm ) so I had to put a ky-005 in it.
![it is](https://raw.githubusercontent.com/kiralyc/standalone_esp_AC_server/main/project.jpg)

I have used the onboard RGB led from M5 Atom but it is just some basic feedback just for fun, while the AC is on it is marking the current AC Mode state via color.

The device is surprisingly responsive thanks for the websocket functionality and the static content which comes from the SPIFF. (if you want to try this code do not forget the upload the static content from data directory to esp via Filesystem Uploader Plugin and setup a portforward in your router if using from internet via duckdns)

I using MideaHeatpumpIR library from heatpumpir because I have a midea AC but you can change change it to any supported lib which you can find in heatpumpir.

I use this server in a custom port behind my firewall (if you want to change the standard 80 port to a custom one then do not forget to put it in the gateway variable in front of the script.js ) 
I think it would be better on https protocol, but I can not solve this issue with my limited skills and time (currently the ESPAsyncWebServer lib does not support https with esp32 and I have no time to rewrite it).
