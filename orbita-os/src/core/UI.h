#pragma once
#include <M5Cardputer.h>

#include "App.h"

// Utilidades de desenho compartilhadas pelos apps.
namespace ui {

// Fontes (todas ASCII, ja inclusas no M5GFX)
const lgfx::IFont* small();   // 6x8
const lgfx::IFont* body();    // ~16 px
const lgfx::IFont* title();   // negrito ~13 px

// Lista com rolagem
struct ListView {
  int sel = 0;
  int top = 0;
  int count = 0;
  int rowH = 18;

  void setCount(int n);
  // Retorna true se a selecao mudou
  bool handle(const KeyEvent& e, int visibleRows);
  int visibleRows(const Rect& area) const { return area.h / rowH; }
  void ensureVisible(int rows);
};

// Desenha as linhas visiveis. drawRow(i, rect, selecionado)
template <typename F>
void drawList(M5Canvas& c, const Rect& area, ListView& lv, F drawRow) {
  int rows = lv.visibleRows(area);
  lv.ensureVisible(rows);
  for (int r = 0; r < rows; r++) {
    int i = lv.top + r;
    if (i >= lv.count) break;
    Rect row{area.x, area.y + r * lv.rowH, area.w, lv.rowH};
    drawRow(i, row, i == lv.sel);
  }
}

void drawScrollbar(M5Canvas& c, const Rect& area, const ListView& lv);
void drawRowBg(M5Canvas& c, const Rect& r, bool selected);
void drawHint(M5Canvas& c, const char* text);  // linha de dicas no rodape
// Texto com quebra de linha por palavra; retorna a altura usada
int drawWrapped(M5Canvas& c, const String& text, int x, int y, int w, int lineH);
String ellipsize(M5Canvas& c, const String& text, int maxW);

// Icones vetoriais simples
enum class Icon : uint8_t {
  Generic, Planet, Wave, Folder, Install, Gear, Chip, Empty, Power, Note, Globe, Usb, Wifi, Grid
};
void drawIcon(M5Canvas& c, Icon icon, int cx, int cy, int size, uint16_t color, uint16_t bg,
              char letter = 0);

}  // namespace ui
