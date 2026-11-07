#include "Hid.h"

#ifndef ORBITA_USB_MSC
#define ORBITA_USB_MSC 0
#endif

// ======================================================================= USB

#if ORBITA_USB_MSC && !ARDUINO_USB_MODE
#include "USB.h"
#include "USBHIDKeyboard.h"
#define ORBITA_HAS_USB_HID 1
// Objeto global: registra a interface de teclado antes da enumeracao do USB
// (junto com o disco do modo pendrive e a serial).
static USBHIDKeyboard usbKeyboard;
#else
#define ORBITA_HAS_USB_HID 0
#endif

// ======================================================================= BLE

#if __has_include(<BLEDevice.h>) && defined(CONFIG_BT_ENABLED)
#define ORBITA_HAS_BLE 1
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEServer.h>
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <esp_gap_ble_api.h>
#endif
#else
#define ORBITA_HAS_BLE 0
#endif

namespace hid {

#if ORBITA_HAS_BLE

// Teclado simples: report 1 = [modificadores, reservado, 6 teclas]; saida = LEDs
static const uint8_t REPORT_MAP[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0, 0x29, 0xE7,  // Usage Min/Max (modificadores)
    0x15, 0x00, 0x25, 0x01,  // Logical Min 0 / Max 1
    0x75, 0x01, 0x95, 0x08,  // 8 bits
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0x95, 0x01, 0x75, 0x08,
    0x81, 0x01,        //   Input (Const) - reservado
    0x95, 0x05, 0x75, 0x01,
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01, 0x29, 0x05,
    0x91, 0x02,        //   Output (Data, Var, Abs)
    0x95, 0x01, 0x75, 0x03,
    0x91, 0x01,        //   Output (Const) - enchimento
    0x95, 0x06, 0x75, 0x08,
    0x15, 0x00, 0x25, 0x65,
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00, 0x29, 0x65,
    0x81, 0x00,        //   Input (Data, Array)
    0xC0               // End Collection
};

static BLEServer* server = nullptr;
static BLEHIDDevice* hidDev = nullptr;
static BLECharacteristic* input = nullptr;
static BLECharacteristic* output = nullptr;
static volatile bool connected = false;
static bool advertising = false;
static bool started = false;

class ServerCb : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {
    (void)s;
    connected = true;
  }
  void onDisconnect(BLEServer* s) override {
    (void)s;
    connected = false;
    if (advertising) BLEDevice::startAdvertising();  // volta a aceitar conexao
  }
};

bool startBle(String& err) {
  (void)err;
  if (!started) {
    BLEDevice::init(BLE_NAME);
    server = BLEDevice::createServer();
    server->setCallbacks(new ServerCb());

    hidDev = new BLEHIDDevice(server);
    input = hidDev->inputReport(1);
    output = hidDev->outputReport(1);
    (void)output;
    hidDev->manufacturer()->setValue(String("Orbita"));
    hidDev->pnp(0x02, 0xE502, 0xA111, 0x0210);
    hidDev->hidInfo(0x00, 0x01);

#if defined(CONFIG_BLUEDROID_ENABLED)
    // Pareamento "Just Works" com vinculo: o computador lembra do Cardputer
    esp_ble_auth_req_t auth = ESP_LE_AUTH_BOND;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, sizeof(auth));
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    uint8_t keyInit = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &keyInit, sizeof(keyInit));
    uint8_t keyResp = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &keyResp, sizeof(keyResp));
#endif

    hidDev->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
    hidDev->startServices();
    hidDev->setBatteryLevel(100);

    BLEAdvertising* adv = server->getAdvertising();
    adv->setAppearance(0x03C1);  // HID: teclado
    adv->addServiceUUID(hidDev->hidService()->getUUID());
    adv->setScanResponse(false);
    started = true;
  }
  if (!advertising) {
    advertising = true;
    BLEDevice::startAdvertising();
  }
  return true;
}

void stopBle() {
  if (!started) return;
  advertising = false;
  BLEDevice::stopAdvertising();
  if (connected && server) server->disconnect(server->getConnId());
  connected = false;
}

bool bleSupported() { return true; }
bool bleOn() { return started && advertising; }
bool bleConnected() { return connected; }

static bool bleSend(uint8_t usage, uint8_t modifiers) {
  if (!connected || !input) return false;
  uint8_t rpt[8] = {modifiers, 0, usage, 0, 0, 0, 0, 0};
  input->setValue(rpt, sizeof(rpt));
  input->notify();
  delay(12);
  memset(rpt, 0, sizeof(rpt));
  input->setValue(rpt, sizeof(rpt));
  input->notify();
  return true;
}

void setBattery(uint8_t pct) {
  if (started && hidDev) hidDev->setBatteryLevel(pct > 100 ? 100 : pct);
}

#else  // sem Bluetooth nesta compilacao

bool startBle(String& err) {
  err = "Compilado sem Bluetooth";
  return false;
}
void stopBle() {}
bool bleSupported() { return false; }
bool bleOn() { return false; }
bool bleConnected() { return false; }
static bool bleSend(uint8_t, uint8_t) { return false; }
void setBattery(uint8_t) {}

#endif

// ---------------------------------------------------------------- USB

#if ORBITA_HAS_USB_HID
void beginUsb() { usbKeyboard.begin(); }
bool usbSupported() { return true; }
bool usbConnected() { return (bool)USB; }

static bool usbSend(uint8_t usage, uint8_t modifiers) {
  if (!usbConnected()) return false;
  // pressRaw trata 0xE0..0xE7 como modificadores
  for (uint8_t b = 0; b < 8; b++)
    if (modifiers & (1 << b)) usbKeyboard.pressRaw(0xE0 + b);
  usbKeyboard.pressRaw(usage);
  delay(12);
  usbKeyboard.releaseAll();
  return true;
}
#else
void beginUsb() {}
bool usbSupported() { return false; }
bool usbConnected() { return false; }
static bool usbSend(uint8_t, uint8_t) { return false; }
#endif

bool anyConnected() { return bleConnected() || usbConnected(); }

bool tap(uint8_t usage, uint8_t modifiers) {
  bool a = bleSend(usage, modifiers);
  bool b = usbSend(usage, modifiers);
  return a || b;
}

}  // namespace hid
