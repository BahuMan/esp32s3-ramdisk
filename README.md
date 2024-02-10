This is adapted from the "msc ramdisk" example in the TinyUSB library in PlatformIO.
I had to make the following changes:

### in platformio.ini
	-DARDUINO_USB_MODE=0
	-DARDUINO_USB_CDC_ON_BOOT=1
	-DCORE_DEBUG_LEVEL=1

### in main.cpp
> #include <Arduino.h>
> #include <SD.h>
>
> Serial.setDebugOutput(true);
