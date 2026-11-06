#pragma once
#include "../core/App.h"
#include "../core/UI.h"

// "Apps": gerencia os firmwares completos dos slots (Hazke, GLIDE...).
// Abrir (troca de firmware), substituir, renomear, apagar e escolher qual abre ao ligar.
class FirmwareApp : public App {
 public:
  const char* title() const override { return "Apps (firmwares)"; }
  void onEnter() override;
  void onResume() override { onEnter(); }
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  void slotMenu(int slot);
  ui::ListView lv_;  // slots... + "Instalar do cartao SD"
};
