#pragma once
#include <functional>
#include <vector>

#include "../core/App.h"
#include "../core/UI.h"

// Tela inicial: carrossel com os apps instalados nos slots e os apps nativos.
class HomeApp : public App {
 public:
  const char* title() const override { return "Inicio"; }
  void onEnter() override { rebuild(); }
  void onResume() override { rebuild(); }
  void onKey(const KeyEvent& e) override;
  void update(uint32_t now) override;
  void render(M5Canvas& c, const Rect& area) override;
  bool animating() const override { return animating_; }

 private:
  struct Item {
    String label;
    String sub;
    ui::Icon icon;
    char letter;
    int slot;                      // >= 0: app externo neste slot
    std::function<void()> open;
  };

  void rebuild();
  void slotMenu(int slot);

  std::vector<Item> items_;
  int sel_ = 0;
  float pos_ = 0;       // posicao animada do carrossel
  bool animating_ = false;
  uint32_t lastStep_ = 0;
};
