# ESP32_C3_DevKitM_1
Arduino sketches for ESP32 C3

So first install correct USB drivers for windows: CP210x. This is a zip-file and via Device Manager (Apparaatbeheer) update the driver for USB.

Than download Arduino . I used the old Arduino IDE, but newest shoud work also.
Install ESP32 boards from espressif. But choose 2.0.0 and not newer. The ESP32-c3 in the devkit is no longer supported in the newer versions. You will see this as a bootloop in the Serial monitor. Something like this will show:  
ESP-ROM:esp32c3-api1-20210207  
Build:Feb 7 2021  
rst:0x1 (POWERON),  
boot:0xc (SPI_FAST_FLASH_BOOT)  
SPIWP:0xee  
mode:QIO,  
clock div:1  
load:0x3fcd5810,  
len:0x438  
ets_loader.c  

See also: https://github.com/espressif/arduino-esp32/issues/6013
