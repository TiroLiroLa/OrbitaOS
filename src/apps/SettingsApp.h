#pragma once
#include "../core/App.h"
#include "../core/UI.h"

class SettingsApp : public App {
 public:
  const char* title() const override { return "Ajustes"; }
  void onEnter() override;
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  enum Row { Brightness, Dim, Accent, Sound, Reboot, RowCount };
  void change(int row, int dir);  // dir: -1, +1, ou 0 (Enter)
  String valueText(int row) const;
  ui::ListView lv_;
};
