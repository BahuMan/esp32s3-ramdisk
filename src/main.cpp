#include <Arduino.h>
//#include <SD.h>
#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

// ESP32 use same flash device that store code.
// Therefore there is no need to specify the SPI and SS
Adafruit_FlashTransport_ESP32 flashTransport;
Adafruit_SPIFlash flash(&flashTransport);

// file system object from SdFat
FatVolume fatfs;

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;

bool fs_formatted;  // Check if flash is formatted
bool fs_changed;    // Set to true when browser write to flash


// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_callback (uint32_t lba, void* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.readBlocks(lba, (uint8_t*) buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_callback (uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
#ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, HIGH);
#endif

  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_callback (void)
{
  // sync with flash
  flash.syncBlocks();

  // clear file system's cache to force refresh
  fatfs.cacheClear();

#ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, LOW);
#endif
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool msc_ready_callback(void)
{
  // if fs has changed, mark unit as not ready temporarily to force PC to flush cache
  bool ret = !fs_changed;
  fs_changed = false;
  return ret;
}

void setupMassStorage(void)
{
  flash.begin();

  // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
  usb_msc.setID("Fri3d", "Badge Flash", "2024");

  // Set callback
  usb_msc.setReadWriteCallback(msc_read_callback, msc_write_callback, msc_flush_callback);

  // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setCapacity(flash.size()/512, 512);

  // MSC is ready for read/write
  fs_changed = false;
  usb_msc.setReadyCallback(0, msc_ready_callback);

  usb_msc.begin();

  // Init file system on the flash
  fs_formatted = fatfs.begin(&flash);
}

void listFiles()
{
  File32 root = fatfs.open("/");
  File32 file = root.openNextFile();
  char buf[256];
  while (file)
  {
    Serial.print("Content for file '");
    file.getName(buf, 256);
    Serial.print(buf);
    Serial.println("':");

    int l = 0;
    do {
      l = file.readBytes(buf, 255);
      buf[l] = '\0';
      Serial.print(buf);
    } while (l > 0);

    Serial.println();
    Serial.println();
    file = root.openNextFile();
  }
}
// the setup function runs once when you press reset or power the board
void setup() {

  setupMassStorage();

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(500);   // wait for native usb
  if ( !fs_formatted )
  {
    Serial.println("Failed to init files system, flash may not be formatted");
  }
  else {
    Serial.println("Fri3d Camp Mass Storage initialized");
  }

  listFiles();
}

void loop()
{
  listFiles();
  delay(2000);
}
