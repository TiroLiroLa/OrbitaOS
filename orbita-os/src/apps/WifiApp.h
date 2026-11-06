#pragma once
#include <vector>

#include "../core/App.h"
#include "../core/Net.h"
#include "../core/UI.h"

// Wi-Fi: procurar redes, conectar (lembra a senha), esquecer, criar rede propria.
class WifiApp : public App {
 public:
  const char* title() const override { return "Wi-Fi"; }
  void onEnter() override;
  void onKey(const KeyEvent& e) override;
  void update(uint32_t now) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  enum Fixed { Scan, AP, Off, FIXED_COUNT };
  void rebuild();
  void connectTo(const net::ScanResult& r);

  std::vector<net::ScanResult> list_;
  ui::ListView lv_;
  net::State lastState_ = net::State::Off;
  bool wasScanning_ = false;
  uint32_t lastPoll_ = 0;
};
