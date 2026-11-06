#pragma once
#include <stdint.h>

// Cores em RGB565. Guardadas como uint16_t de proposito: o M5GFX interpreta
// uint16_t como RGB565, uint32_t como RGB888 e uint8_t como RGB332.
struct Theme {
  uint16_t bg;          // fundo
  uint16_t panel;       // caixas, cartoes
  uint16_t fg;          // texto principal
  uint16_t dim;         // texto secundario
  uint16_t accent;      // cor de destaque
  uint16_t accentDark;  // destaque escurecido (selecao)
  uint16_t warn;
  uint16_t ok;
};

namespace theme {

struct AccentDef {
  const char* name;
  uint8_t r, g, b;
};

constexpr int ACCENT_COUNT = 6;
extern const AccentDef ACCENTS[ACCENT_COUNT];

inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

void apply(uint8_t accentIndex);
const Theme& get();

}  // namespace theme
