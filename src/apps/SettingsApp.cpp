#include "SettingsApp.h"

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"

static const uint16_t DIM_STEPS[] = {0, 15, 30, 60, 120, 300};
static constexpr int DIM_COUNT = sizeof(DIM_STEPS) / sizeof(DIM_STEPS[0]);

static const char* LABELS[] = {"Brilho", "Escurecer tela", "Cor de destaque", "Sons do sistema",
                               "Reiniciar o Orbita"};

void SettingsApp::onEnter() {
  lv_.setCount(RowCount);
  lv_.rowH = 18;
}

String SettingsApp::valueText(int row) const {
  const Settings& s = Kernel::get().settings();
  switch (row) {
    case Brightness: return String(s.brightness * 100 / 255) + "%";
    case Dim:        return s.dimSecs == 0 ? String("nunca")
                          : s.dimSecs < 60 ? String(s.dimSecs) + "s"
                                           : String(s.dimSecs / 60) + "min";
    case Accent:     return theme::ACCENTS[s.accent % theme::ACCENT_COUNT].name;
    case Sound:      return s.sound ? "sim" : "nao";
    default:         return "";
  }
}

void SettingsApp::change(int row, int dir) {
  Kernel& k = Kernel::get();
  Settings& s = k.settings();
  switch (row) {
    case Brightness: {
      int v = s.brightness + (dir == 0 ? 32 : dir * 16);
      if (dir == 0 && s.brightness >= 255) v = 16;  // Enter da a volta
      if (v < 16) v = 16;
      if (v > 255) v = 255;
      s.brightness = (uint8_t)v;
      k.applyBrightness();
      break;
    }
    case Dim: {
      int idx = 0;
      for (int i = 0; i < DIM_COUNT; i++)
        if (DIM_STEPS[i] == s.dimSecs) idx = i;
      idx = (idx + (dir < 0 ? -1 : 1) + DIM_COUNT) % DIM_COUNT;
      s.dimSecs = DIM_STEPS[idx];
      break;
    }
    case Accent:
      s.accent = (uint8_t)((s.accent + (dir < 0 ? -1 : 1) + theme::ACCENT_COUNT) %
                           theme::ACCENT_COUNT);
      k.applyTheme();
      break;
    case Sound:
      s.sound = !s.sound;
      break;
    case Reboot:
      if (dir == 0) {
        k.confirm("Reiniciar?", "O Orbita sera reiniciado.", []() { Kernel::get().rebootToOS(); },
                  "Reiniciar", "Cancelar");
      }
      return;
  }
  s.save();
  invalidate();
}

void SettingsApp::onKey(const KeyEvent& e) {
  if (e.nav == Nav::Up || e.nav == Nav::Down) {
    if (lv_.handle(e, 6)) invalidate();
  } else if (e.nav == Nav::Left) {
    change(lv_.sel, -1);
  } else if (e.nav == Nav::Right) {
    change(lv_.sel, +1);
  } else if (e.nav == Nav::Enter) {
    change(lv_.sel, 0);
  } else if (e.nav == Nav::Back) {
    Kernel::get().pop();
  }
}

void SettingsApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  Rect list{area.x, area.y + 2, area.w, area.h - 14};
  ui::drawList(c, list, lv_, [&](int i, const Rect& r, bool sel) {
    ui::drawRowBg(c, r, sel);
    c.setFont(ui::small());
    c.setTextColor(sel ? t.fg : t.dim);
    c.drawString(LABELS[i], r.x + 10, r.y + 5);
    String v = valueText(i);
    if (v.length()) {
      c.setTextColor(sel ? t.accent : t.dim);
      c.setTextDatum(top_right);
      String shown = v;
      if (sel) shown = "< " + v + " >";
      c.drawString(shown.c_str(), r.x + r.w - 10, r.y + 5);
      c.setTextDatum(top_left);
    }
    if (i == Accent) {  // amostra da cor
      c.fillCircle(r.x + r.w - (sel ? 78 : 62), r.y + r.h / 2, 3, t.accent);
    }
  });
  ui::drawHint(c, "<> mudar   OK alternar   ` voltar");
}
