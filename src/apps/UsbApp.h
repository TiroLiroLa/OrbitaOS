#pragma once
#include "../core/App.h"

// Modo pendrive: o cartao SD aparece no computador pelo cabo USB-C.
class UsbApp : public App {
 public:
  const char* title() const override { return "USB (pendrive)"; }
  void onEnter() override;
  void onExit() override;
  void onKey(const KeyEvent& e) override;
  void update(uint32_t now) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  bool ok_ = false;
  String err_;
  uint32_t lastPoll_ = 0;
};
