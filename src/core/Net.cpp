#include "Net.h"

#include <WiFi.h>
#include <algorithm>
#include <esp_random.h>

#include "Config.h"
#include "KvFile.h"

namespace net {

static State st = State::Off;
static std::vector<Known> knownList;
static std::vector<ScanResult> scanList;
static bool autoAfterScan = false;
static String pendingSsid, pendingPass;
static uint32_t connectStarted = 0;
static String apName, apPass, err;

// ------------------------------------------------------------ redes salvas

static void loadKnown() {
  knownList.clear();
  KvFile kv(cfg::FILE_WIFI);
  kv.load();
  for (const auto& it : kv.items()) {
    if ((int)knownList.size() >= cfg::MAX_KNOWN_WIFI) break;
    knownList.push_back({it.key, it.value});
  }
}

static void saveKnown() {
  KvFile kv(cfg::FILE_WIFI);
  for (const auto& k : knownList) kv.set(k.ssid, k.pass);
  kv.save();
}

static void remember(const String& s, const String& pw) {
  for (size_t i = 0; i < knownList.size(); i++) {
    if (knownList[i].ssid == s) {
      knownList.erase(knownList.begin() + i);
      break;
    }
  }
  knownList.insert(knownList.begin(), {s, pw});  // mais recente primeiro
  while ((int)knownList.size() > cfg::MAX_KNOWN_WIFI) knownList.pop_back();
  saveKnown();
}

static const Known* findKnown(const String& s) {
  for (const auto& k : knownList)
    if (k.ssid == s) return &k;
  return nullptr;
}

// ------------------------------------------------------------ API

void begin() {
  WiFi.persistent(false);  // o Orbita guarda as redes dele; nada na NVS do Wi-Fi
  loadKnown();
}

State state() { return st; }

const char* stateText() {
  switch (st) {
    case State::Off:         return "desligado";
    case State::Scanning:    return "procurando redes";
    case State::Connecting:  return "conectando";
    case State::Connected:   return "conectado";
    case State::AccessPoint: return "rede propria";
    case State::Failed:      return "falhou";
  }
  return "";
}

bool online() { return st == State::Connected || st == State::AccessPoint; }

String ip() {
  if (st == State::Connected) return WiFi.localIP().toString();
  if (st == State::AccessPoint) return WiFi.softAPIP().toString();
  return "";
}

String ssid() {
  if (st == State::AccessPoint) return apName;
  if (st == State::Connected) return WiFi.SSID();
  if (st == State::Connecting) return pendingSsid;
  return "";
}

String apPassword() { return st == State::AccessPoint ? apPass : String(""); }
String lastError() { return err; }

void startScan() {
  if (st == State::AccessPoint) return;
  if (WiFi.getMode() != WIFI_STA) WiFi.mode(WIFI_STA);
  WiFi.scanDelete();
  WiFi.scanNetworks(true);  // assincrono
  if (st != State::Connected) st = State::Scanning;
  scanList.clear();
}

bool scanning() { return WiFi.scanComplete() == WIFI_SCAN_RUNNING; }

const std::vector<ScanResult>& results() { return scanList; }

void connect(const String& s, const String& pw) {
  err = "";
  if (st == State::AccessPoint) WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(cfg::HOSTNAME);
  WiFi.disconnect();
  pendingSsid = s;
  pendingPass = pw;
  if (pw.length()) WiFi.begin(s.c_str(), pw.c_str());
  else WiFi.begin(s.c_str());
  connectStarted = millis();
  st = State::Connecting;
}

bool connectKnown() {
  if (knownList.empty()) return false;
  autoAfterScan = true;
  startScan();
  return true;
}

void startAP() {
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char name[20];
  snprintf(name, sizeof(name), "Orbita-%02X%02X", mac[4], mac[5]);
  apName = name;
  char pw[12];
  snprintf(pw, sizeof(pw), "%08lu", (unsigned long)(esp_random() % 100000000UL));
  apPass = pw;
  if (WiFi.softAP(apName.c_str(), apPass.c_str())) {
    st = State::AccessPoint;
  } else {
    st = State::Failed;
    err = "Nao criou a rede";
  }
}

void stop() {
  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  st = State::Off;
  autoAfterScan = false;
}

const std::vector<Known>& known() { return knownList; }

void forget(const String& s) {
  for (size_t i = 0; i < knownList.size(); i++) {
    if (knownList[i].ssid == s) {
      knownList.erase(knownList.begin() + i);
      saveKnown();
      return;
    }
  }
}

void update() {
  // Resultado da busca
  int n = WiFi.scanComplete();
  if (n >= 0) {
    scanList.clear();
    for (int i = 0; i < n; i++) {
      String s = WiFi.SSID(i);
      if (s.length() == 0) continue;  // rede oculta
      bool dup = false;
      for (auto& r : scanList)
        if (r.ssid == s) { dup = true; break; }
      if (dup) continue;
      scanList.push_back({s, (int)WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN,
                          findKnown(s) != nullptr});
    }
    WiFi.scanDelete();  // libera a memoria; a lista fica em scanList
    std::sort(scanList.begin(), scanList.end(),
              [](const ScanResult& a, const ScanResult& b) { return a.rssi > b.rssi; });
    if (st == State::Scanning) st = State::Off;

    if (autoAfterScan) {
      autoAfterScan = false;
      for (const auto& r : scanList) {  // ja ordenado pelo sinal
        const Known* k = findKnown(r.ssid);
        if (k) {
          connect(k->ssid, k->pass);
          return;
        }
      }
      st = State::Failed;
      err = "Nenhuma rede conhecida por perto";
    }
  }

  if (st == State::Connecting) {
    wl_status_t w = WiFi.status();
    if (w == WL_CONNECTED) {
      st = State::Connected;
      remember(pendingSsid, pendingPass);
      pendingPass = "";
    } else if (w == WL_CONNECT_FAILED || millis() - connectStarted > cfg::WIFI_TIMEOUT_MS) {
      st = State::Failed;
      err = (w == WL_CONNECT_FAILED) ? "Senha errada?" : "Tempo esgotado";
      WiFi.disconnect();
    }
  } else if (st == State::Connected && WiFi.status() != WL_CONNECTED) {
    st = State::Failed;
    err = "Conexao caiu";
  }
}

}  // namespace net
