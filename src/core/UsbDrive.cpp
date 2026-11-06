#include "UsbDrive.h"

#include "Storage.h"

#ifndef ORBITA_USB_MSC
#define ORBITA_USB_MSC 0
#endif

#if ORBITA_USB_MSC && !ARDUINO_USB_MODE

#include <SD.h>

#include "USB.h"
#include "USBMSC.h"

namespace usbdrive {

// Objeto global: o construtor registra a interface de disco no TinyUSB antes
// do USB ser iniciado (a lista de interfaces e fixada na enumeracao).
static USBMSC msc;
static volatile bool isActive = false, isEjected = false, seen = false;
static volatile uint32_t rd = 0, wr = 0;
static uint32_t secSize = 512;
static uint8_t* tmp = nullptr;  // setor temporario para acessos parciais

// Le/escreve um trecho que pode comecar no meio de um setor
static bool xfer(uint32_t lba, uint32_t offset, uint8_t* buf, uint32_t len, bool write) {
  while (len > 0) {
    lba += offset / secSize;
    offset %= secSize;
    uint32_t chunk = secSize - offset;
    if (chunk > len) chunk = len;
    if (offset == 0 && chunk == secSize) {
      bool ok = write ? SD.writeRAW(buf, lba) : SD.readRAW(buf, lba);
      if (!ok) return false;
    } else {
      if (!tmp) return false;
      if (!SD.readRAW(tmp, lba)) return false;
      if (write) {
        memcpy(tmp + offset, buf, chunk);
        if (!SD.writeRAW(tmp, lba)) return false;
      } else {
        memcpy(buf, tmp + offset, chunk);
      }
    }
    buf += chunk;
    len -= chunk;
    lba += 1;
    offset = 0;
  }
  return true;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  if (!isActive) return -1;
  seen = true;
  if (!xfer(lba, offset, (uint8_t*)buffer, bufsize, false)) return -1;
  rd += bufsize;
  return (int32_t)bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  if (!isActive) return -1;
  seen = true;
  if (!xfer(lba, offset, buffer, bufsize, true)) return -1;
  wr += bufsize;
  return (int32_t)bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  (void)power_condition;
  if (load_eject && !start) isEjected = true;
  return true;
}

void begin() {
  if (!tmp) tmp = (uint8_t*)malloc(512);
  msc.vendorID("Orbita");
  msc.productID("Cartao SD");
  msc.productRevision("1.0");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.onStartStop(onStartStop);
  msc.mediaPresent(false);
  msc.begin(0, 512);
  USB.begin();  // se o core ja iniciou o USB (serial), nao faz nada
}

bool supported() { return true; }

bool start(String& err) {
  if (isActive) return true;
  if (!storage::begin()) {
    err = "Sem cartao SD";
    return false;
  }
  secSize = SD.sectorSize();
  if (secSize != 512) {
    err = "Setor de " + String(secSize) + " bytes nao suportado";
    return false;
  }
  uint32_t count = SD.numSectors();
  if (count == 0) {
    err = "Nao leu o tamanho do cartao";
    return false;
  }
  storage::lockSD(true);  // daqui em diante o sistema nao toca no FAT
  rd = wr = 0;
  isEjected = false;
  seen = false;
  msc.begin(count, (uint16_t)secSize);
  isActive = true;
  msc.mediaPresent(true);
  return true;
}

void stop() {
  if (!isActive) return;
  msc.mediaPresent(false);
  delay(100);
  isActive = false;
  storage::lockSD(false);
  storage::remountSD();  // descarta o cache do FAT, o PC pode ter mudado tudo
}

bool active() { return isActive; }
bool ejected() { return isEjected; }
bool hostSeen() { return seen; }
uint32_t bytesRead() { return rd; }
uint32_t bytesWritten() { return wr; }

}  // namespace usbdrive

#else  // sem suporte a pendrive nesta compilacao

namespace usbdrive {
void begin() {}
bool supported() { return false; }
bool start(String& err) {
  err = "Compilado sem modo pendrive (use o env cardputer-adv)";
  return false;
}
void stop() {}
bool active() { return false; }
bool ejected() { return false; }
bool hostSeen() { return false; }
uint32_t bytesRead() { return 0; }
uint32_t bytesWritten() { return 0; }
}  // namespace usbdrive

#endif
