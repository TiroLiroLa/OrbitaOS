#pragma once
#include <vector>

#include "../core/App.h"
#include "../core/UI.h"

// Informacoes do sistema: bootloader, memoria, particoes, SD, bateria.
class SystemApp : public App {
 public:
  const char* title() const override { return "Sistema"; }
  void onEnter() override { collect(); }
  void onKey(const KeyEvent& e) override;
  void update(uint32_t now) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  struct Line {
    String key, value;
    bool header;
  };
  void collect();
  std::vector<Line> lines_;
  int top_ = 0;
  uint32_t lastRefresh_ = 0;
};
