#include "UI.h"

#include <math.h>

#include "Config.h"
#include "Theme.h"

namespace ui {

const lgfx::IFont* small() { return &fonts::Font0; }
const lgfx::IFont* body() { return &fonts::Font2; }
const lgfx::IFont* title() { return &fonts::FreeSansBold9pt7b; }

// ---------------------------------------------------------------- ListView

void ListView::setCount(int n) {
  count = n;
  if (sel >= count) sel = count > 0 ? count - 1 : 0;
  if (sel < 0) sel = 0;
  if (top > sel) top = sel;
}

void ListView::ensureVisible(int rows) {
  if (rows <= 0) return;
  if (sel < top) top = sel;
  if (sel >= top + rows) top = sel - rows + 1;
  if (top < 0) top = 0;
}

bool ListView::handle(const KeyEvent& e, int rows) {
  if (count <= 0) return false;
  int old = sel;
  switch (e.nav) {
    case Nav::Up:
      sel = (sel - 1 + count) % count;
      break;
    case Nav::Down:
      sel = (sel + 1) % count;
      break;
    case Nav::Left:  // pagina acima
      sel -= rows > 1 ? rows - 1 : 1;
      if (sel < 0) sel = 0;
      break;
    case Nav::Right:  // pagina abaixo
      sel += rows > 1 ? rows - 1 : 1;
      if (sel >= count) sel = count - 1;
      break;
    default:
      return false;
  }
  ensureVisible(rows);
  return sel != old;
}

// ---------------------------------------------------------------- desenho

void drawRowBg(M5Canvas& c, const Rect& r, bool selected) {
  const Theme& t = theme::get();
  if (selected) {
    c.fillRoundRect(r.x + 2, r.y + 1, r.w - 4, r.h - 2, 4, t.accentDark);
    c.fillRect(r.x + 2, r.y + 3, 2, r.h - 6, t.accent);
  }
}

void drawScrollbar(M5Canvas& c, const Rect& area, const ListView& lv) {
  int rows = lv.visibleRows(area);
  if (lv.count <= rows || lv.count == 0) return;
  const Theme& t = theme::get();
  int x = area.x + area.w - 3;
  int barH = area.h * rows / lv.count;
  if (barH < 6) barH = 6;
  int maxTop = lv.count - rows;
  int y = area.y + (area.h - barH) * lv.top / (maxTop > 0 ? maxTop : 1);
  c.fillRect(x, area.y, 2, area.h, t.panel);
  c.fillRect(x, y, 2, barH, t.accent);
}

void drawHint(M5Canvas& c, const char* text) {
  const Theme& t = theme::get();
  int y = cfg::SCREEN_H - 10;
  c.fillRect(0, y - 1, cfg::SCREEN_W, 11, t.panel);
  c.setFont(small());
  c.setTextColor(t.dim, t.panel);
  c.setTextDatum(top_center);
  c.drawString(text, cfg::SCREEN_W / 2, y + 1);
  c.setTextDatum(top_left);
}

String ellipsize(M5Canvas& c, const String& text, int maxW) {
  if (c.textWidth(text.c_str()) <= maxW) return text;
  String s = text;
  while (s.length() > 1 && c.textWidth((s + "...").c_str()) > maxW) s.remove(s.length() - 1);
  return s + "...";
}

int drawWrapped(M5Canvas& c, const String& text, int x, int y, int w, int lineH) {
  int cy = y;
  String line;
  int start = 0;
  const int n = text.length();
  while (start <= n) {
    int sp = text.indexOf(' ', start);
    int nl = text.indexOf('\n', start);
    int end = sp;
    bool hardBreak = false;
    if (nl >= 0 && (sp < 0 || nl < sp)) { end = nl; hardBreak = true; }
    if (end < 0) end = n;
    String word = text.substring(start, end);
    String trial = line.length() ? line + " " + word : word;
    if (line.length() && c.textWidth(trial.c_str()) > w) {
      c.drawString(line.c_str(), x, cy);
      cy += lineH;
      line = word;
    } else {
      line = trial;
    }
    if (hardBreak) {
      c.drawString(line.c_str(), x, cy);
      cy += lineH;
      line = "";
    }
    start = end + 1;
  }
  if (line.length()) {
    c.drawString(line.c_str(), x, cy);
    cy += lineH;
  }
  return cy - y;
}

// ---------------------------------------------------------------- icones

static void drawGear(M5Canvas& c, int cx, int cy, int s, uint16_t col, uint16_t bg) {
  int rOut = s * 42 / 100, rIn = s * 30 / 100;
  for (int k = 0; k < 8; k++) {
    float a = k * (float)M_PI / 4;
    int x0 = cx + (int)(cosf(a) * rIn), y0 = cy + (int)(sinf(a) * rIn);
    int x1 = cx + (int)(cosf(a) * rOut), y1 = cy + (int)(sinf(a) * rOut);
    c.drawWideLine(x0, y0, x1, y1, s / 12 + 1, col);
  }
  c.fillCircle(cx, cy, rIn, col);
  c.fillCircle(cx, cy, s * 12 / 100, bg);
}

void drawIcon(M5Canvas& c, Icon icon, int cx, int cy, int s, uint16_t col, uint16_t bg,
              char letter) {
  const int h = s / 2;
  switch (icon) {
    case Icon::Planet: {
      int r = s * 26 / 100;
      c.fillCircle(cx, cy, r, col);
      c.fillCircle(cx - r / 3, cy - r / 3, r / 4, bg);  // cratera
      c.drawEllipse(cx, cy, s * 46 / 100, s * 13 / 100, col);
      c.drawEllipse(cx, cy, s * 46 / 100 - 1, s * 13 / 100 - 1, col);
      break;
    }
    case Icon::Wave: {
      int px = cx - h * 85 / 100, py = cy;
      for (int x = -h * 85 / 100; x <= h * 85 / 100; x += 2) {
        int y = cy + (int)(sinf(x * 6.28f / (s * 0.55f)) * s * 0.22f);
        c.drawWideLine(px, py, cx + x, y, s / 16 + 1, col);
        px = cx + x;
        py = y;
      }
      break;
    }
    case Icon::Folder: {
      int w = s * 80 / 100, hh = s * 56 / 100;
      int x = cx - w / 2, y = cy - hh / 2 + s / 12;
      c.fillRoundRect(x, y - s / 10, w * 45 / 100, s / 5, 3, col);
      c.fillRoundRect(x, y, w, hh, 4, col);
      c.fillRect(x + 3, y + 4, w - 6, 2, bg);
      break;
    }
    case Icon::Install: {
      int w = s * 70 / 100, lw = s / 12 + 1;
      c.drawWideLine(cx, cy - h * 70 / 100, cx, cy + s / 10, lw, col);
      c.drawWideLine(cx - s / 6, cy - s / 12, cx, cy + s / 10, lw, col);
      c.drawWideLine(cx + s / 6, cy - s / 12, cx, cy + s / 10, lw, col);
      int ty = cy + h * 60 / 100;
      c.drawWideLine(cx - w / 2, ty - s / 8, cx - w / 2, ty, lw, col);
      c.drawWideLine(cx - w / 2, ty, cx + w / 2, ty, lw, col);
      c.drawWideLine(cx + w / 2, ty, cx + w / 2, ty - s / 8, lw, col);
      break;
    }
    case Icon::Gear:
      drawGear(c, cx, cy, s, col, bg);
      break;
    case Icon::Chip: {
      int w = s * 50 / 100;
      for (int k = -1; k <= 1; k++) {
        int o = k * w / 3;
        c.fillRect(cx + o - 1, cy - w / 2 - s / 8, 3, s / 8, col);
        c.fillRect(cx + o - 1, cy + w / 2, 3, s / 8, col);
        c.fillRect(cx - w / 2 - s / 8, cy + o - 1, s / 8, 3, col);
        c.fillRect(cx + w / 2, cy + o - 1, s / 8, 3, col);
      }
      c.fillRoundRect(cx - w / 2, cy - w / 2, w, w, 3, col);
      c.drawRoundRect(cx - w / 4, cy - w / 4, w / 2, w / 2, 2, bg);
      break;
    }
    case Icon::Power: {
      int r = s * 32 / 100, lw = s / 12 + 1;
      // arco aberto em cima (0 graus = direita, sentido horario)
      c.fillArc(cx, cy, r, r - lw, 300, 360, col);
      c.fillArc(cx, cy, r, r - lw, 0, 240, col);
      c.drawWideLine(cx, cy - r - 2, cx, cy - r / 4, lw, col);
      break;
    }
    case Icon::Empty: {
      int r = s * 34 / 100, lw = s / 14 + 1;
      for (int k = 0; k < 12; k += 2) c.fillArc(cx, cy, r, r - 2, k * 30, k * 30 + 18, col);
      c.drawWideLine(cx - r / 2, cy, cx + r / 2, cy, lw, col);
      c.drawWideLine(cx, cy - r / 2, cx, cy + r / 2, lw, col);
      break;
    }
    case Icon::Note: {
      int w = s * 56 / 100, hh = s * 70 / 100;
      int x = cx - w / 2, y = cy - hh / 2;
      c.fillRoundRect(x, y, w, hh, 3, col);
      for (int k = 0; k < 4; k++)
        c.fillRect(x + 4, y + 6 + k * (hh - 10) / 4, (k == 3 ? w / 2 : w - 8), 2, bg);
      break;
    }
    case Icon::Globe: {
      int r = s * 36 / 100;
      c.fillCircle(cx, cy, r, col);
      c.drawEllipse(cx, cy, r / 2, r, bg);
      c.drawFastHLine(cx - r, cy, 2 * r, bg);
      c.drawFastHLine(cx - r + 3, cy - r / 2, 2 * r - 6, bg);
      c.drawFastHLine(cx - r + 3, cy + r / 2, 2 * r - 6, bg);
      c.drawFastVLine(cx, cy - r, 2 * r, bg);
      break;
    }
    case Icon::Usb: {
      int lw = s / 14 + 1, top = cy - s * 38 / 100, bot = cy + s * 30 / 100;
      c.drawWideLine(cx, top + s / 10, cx, bot, lw, col);
      c.fillTriangle(cx, top, cx - s / 10, top + s / 8, cx + s / 10, top + s / 8, col);
      c.drawWideLine(cx, cy + s / 12, cx - s / 5, cy - s / 20, lw, col);
      c.drawWideLine(cx - s / 5, cy - s / 20, cx - s / 5, cy - s / 6, lw, col);
      c.fillCircle(cx - s / 5, cy - s / 6, s / 16 + 1, col);
      c.drawWideLine(cx, cy + s / 6, cx + s / 5, cy + s / 20, lw, col);
      c.drawWideLine(cx + s / 5, cy + s / 20, cx + s / 5, cy - s / 10, lw, col);
      c.fillRect(cx + s / 5 - s / 14, cy - s / 10 - s / 7, s / 7, s / 7, col);
      c.fillCircle(cx, bot, s / 10, col);
      break;
    }
    case Icon::Wifi: {
      int base = cy + s * 28 / 100, lw = s / 14 + 1;
      c.fillCircle(cx, base, s / 14 + 1, col);
      for (int k = 1; k <= 3; k++) {
        int r = k * s * 17 / 100;
        c.fillArc(cx, base, r, r - lw, 225, 315, col);
      }
      break;
    }
    case Icon::Grid: {
      int q = s * 28 / 100, g = s / 12;
      for (int yy = 0; yy < 2; yy++)
        for (int xx = 0; xx < 2; xx++)
          c.fillRoundRect(cx - q - g / 2 + xx * (q + g), cy - q - g / 2 + yy * (q + g), q, q, 3,
                          col);
      break;
    }
    case Icon::Slides: {  // tela de projecao com um grafico e um "play"
      int w = s * 76 / 100, hh = s * 50 / 100;
      int x = cx - w / 2, y = cy - hh / 2 - s / 12;
      c.fillRoundRect(x, y, w, hh, 3, col);
      c.fillTriangle(cx - s / 10, y + hh / 4, cx - s / 10, y + hh * 3 / 4, cx + s / 7, y + hh / 2,
                     bg);
      int lw = s / 16 + 1;
      c.drawWideLine(cx, y + hh, cx, y + hh + s / 7, lw, col);
      c.drawWideLine(cx - s / 6, y + hh + s / 5, cx, y + hh + s / 10, lw, col);
      c.drawWideLine(cx + s / 6, y + hh + s / 5, cx, y + hh + s / 10, lw, col);
      break;
    }
    case Icon::Generic:
    default: {
      int r = s * 36 / 100;
      c.fillCircle(cx, cy, r, col);
      if (letter) {
        char str[2] = {letter, 0};
        c.setFont(title());
        c.setTextColor(bg);
        c.setTextDatum(middle_center);
        c.drawString(str, cx, cy + 1);
        c.setTextDatum(top_left);
      }
      break;
    }
  }
}

}  // namespace ui
