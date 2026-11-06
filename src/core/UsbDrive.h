#pragma once
#include <Arduino.h>

// Modo pendrive: o cartao SD aparece no computador como um disco USB.
// Enquanto estiver ativo, o Orbita nao mexe no cartao (storage::lockSD).
namespace usbdrive {

void begin();            // chamado uma vez no boot (registra o disco, sem midia)
bool supported();        // compilado com ORBITA_USB_MSC=1
bool start(String& err); // entrega o cartao ao computador
void stop();             // devolve o cartao ao sistema (remonta)
bool active();
bool ejected();          // o computador ejetou o disco
bool hostSeen();         // o computador ja leu algo
uint32_t bytesRead();
uint32_t bytesWritten();

}  // namespace usbdrive
