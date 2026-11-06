#include "Theme.h"

namespace theme {

const AccentDef ACCENTS[ACCENT_COUNT] = {
    {"Ciano",   0, 200, 255},
    {"Laranja", 255, 140, 20},
    {"Verde",   70, 220, 120},
    {"Magenta", 255, 80, 200},
    {"Amarelo", 255, 210, 40},
    {"Lilas",   170, 130, 255},
};

static Theme current;

void apply(uint8_t idx) {
  if (idx >= ACCENT_COUNT) idx = 0;
  const AccentDef& a = ACCENTS[idx];
  current.bg         = rgb(8, 10, 18);
  current.panel      = rgb(24, 28, 44);
  current.fg         = rgb(235, 238, 245);
  current.dim        = rgb(120, 128, 150);
  current.accent     = rgb(a.r, a.g, a.b);
  current.accentDark = rgb(a.r / 4, a.g / 4, a.b / 4);
  current.warn       = rgb(255, 90, 70);
  current.ok         = rgb(80, 220, 120);
}

const Theme& get() { return current; }

}  // namespace theme
