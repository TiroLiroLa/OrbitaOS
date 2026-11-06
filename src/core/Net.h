#pragma once
#include <Arduino.h>

#include <vector>

// Wi-Fi do Orbita: redes conhecidas (NVS), busca, conexao e modo ponto de acesso.
// Tudo nao-bloqueante: chame net::update() todo loop (o Kernel ja faz isso).
namespace net {

enum class State : uint8_t { Off, Scanning, Connecting, Connected, AccessPoint, Failed };

struct Known {
  String ssid, pass;
};
struct ScanResult {
  String ssid;
  int rssi;
  bool open;
  bool known;
};

void begin();
void update();

State state();
const char* stateText();
bool online();           // conectado a uma rede OU com ponto de acesso ligado
String ip();
String ssid();           // rede atual (ou a do ponto de acesso)
String apPassword();     // senha do ponto de acesso, se ligado
String lastError();

void startScan();
bool scanning();
const std::vector<ScanResult>& results();

void connect(const String& ssid, const String& pass);  // e lembra se der certo
bool connectKnown();     // procura uma rede conhecida e conecta; false se nao ha nenhuma salva
void startAP();          // cria a rede "Orbita-XXXX"
void stop();             // desliga o Wi-Fi (economiza bateria)

const std::vector<Known>& known();
void forget(const String& ssid);

}  // namespace net
