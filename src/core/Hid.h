#pragma once
#include <Arduino.h>

// O Cardputer como teclado do computador (controle de apresentacao).
//   - Bluetooth (BLE HID): aparece como "Orbita Apresentador"; pareie uma vez.
//   - USB: quando ligado pelo cabo (build padrao, com USB em modo OTG).
// As teclas vao para os dois que estiverem conectados.
namespace hid {

// Codigos de tecla do padrao USB HID (usage page 0x07)
namespace key {
constexpr uint8_t kA = 0x04, kB = 0x05, kW = 0x1A;
constexpr uint8_t kEnter = 0x28, kEsc = 0x29, kSpace = 0x2C;
constexpr uint8_t kF5 = 0x3E, kPageUp = 0x4B, kPageDown = 0x4E;
constexpr uint8_t kRight = 0x4F, kLeft = 0x50, kDown = 0x51, kUp = 0x52;
}  // namespace key
namespace mod {
constexpr uint8_t kCtrl = 0x01, kShift = 0x02, kAlt = 0x04, kGui = 0x08;
}
// (nomes com "k" porque a biblioteca do Cardputer define SHIFT, KEY_LEFT... como macros)

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
