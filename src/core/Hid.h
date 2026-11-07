#pragma once
#include <Arduino.h>

// O Cardputer como teclado do computador (controle de apresentacao).
//   - Bluetooth (BLE HID): aparece como "Orbita Apresentador"; pareie uma vez.
//   - USB: quando ligado pelo cabo (so no build padrao, com USB em modo OTG).
// As teclas vao para os dois que estiverem conectados.
namespace hid {

// Codigos de tecla do padrao USB HID (usage page 0x07)
namespace key {
constexpr uint8_t A = 0x04, B = 0x05, W = 0x1A;
constexpr uint8_t ENTER = 0x28, ESC = 0x29, SPACE = 0x2C;
constexpr uint8_t F5 = 0x3E, PAGE_UP = 0x4B, PAGE_DOWN = 0x4E;
constexpr uint8_t RIGHT = 0x4F, LEFT = 0x50, DOWN = 0x51, UP = 0x52;
}  // namespace key
namespace mod {
constexpr uint8_t CTRL = 0x01, SHIFT = 0x02, ALT = 0x04, GUI = 0x08;
}

constexpr const char* BLE_NAME = "Orbita Apresentador";

void beginUsb();          // no boot, antes do USB iniciar (registra o teclado USB)
bool startBle(String& err);  // liga o Bluetooth e comeca a anunciar (so na 1a vez pesa)
void stopBle();           // para de anunciar e desconecta (a pilha fica na memoria)
bool bleSupported();
bool bleOn();
bool bleConnected();
bool usbSupported();
bool usbConnected();
bool anyConnected();

// Aperta e solta uma tecla. Retorna true se foi enviada para algum computador.
bool tap(uint8_t usage, uint8_t modifiers = 0);
void setBattery(uint8_t pct);

}  // namespace hid
