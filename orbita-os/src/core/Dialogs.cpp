#include "Dialogs.h"

#include "Config.h"
#include "Kernel.h"
#include "Theme.h"

static Rect dialogBox(int h) {
  int w = cfg::SCREEN_W - 24;
  return Rect{12, (cfg::SCREEN_H - h) / 2 + 4, w, h};
}

static void drawFrame(M5Canvas& c, const Rect& b, const String& title) {
  const Theme& t = theme::get();
  c.fillRoundRect(b.x + 2, b.y + 2, b.w, b.h, 6, t.bg);  // sombra
  c.fillRoundRect(b.x, b.y, b.w, b.h, 6, t.panel);
  c.drawRoundRect(b.x, b.y, b.w, b.h, 6, t.accent);
  c.setFont(ui::small());
  c.setTextColor(t.accent, t.panel);
  c.drawString(title.c_str(), b.x + 8, b.y + 6);
}

// ---------------------------------------------------------------- Confirm

void ConfirmDialog::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (e.nav == Nav::Left || e.nav == Nav::Right || e.nav == Nav::Tab) {
    yesSelected_ = !yesSelected_;
    invalidate();
  } else if (e.nav == Nav::Enter) {
    k.pop();
    if (yesSelected_ && onYes_) onYes_();
  } else if (e.nav == Nav::Back || e.ch == 'n' || e.ch == 'N') {
    k.pop();
  } else if (e.ch == 's' || e.ch == 'S' || e.ch == 'y' || e.ch == 'Y') {
    k.pop();
    if (onYes_) onYes_();
  }
}

void ConfirmDialog::render(M5Canvas& c, const Rect&) {
  const Theme& t = theme::get();
  Rect b = dialogBox(92);
  drawFrame(c, b, title_);
  c.setFont(ui::small());
  c.setTextColor(t.fg, t.panel);
  ui::drawWrapped(c, msg_, b.x + 8, b.y + 20, b.w - 16, 10);

  int bw = 70, bh = 16, by = b.y + b.h - bh - 6;
  struct Btn { const char* label; bool sel; int x; } btns[2] = {
      {no_, !yesSelected_, b.x + b.w / 2 - bw - 6},
      {yes_, yesSelected_, b.x + b.w / 2 + 6},
  };
  for (auto& bt : btns) {
    c.fillRoundRect(bt.x, by, bw, bh, 4, bt.sel ? t.accent : t.bg);
    c.setTextColor(bt.sel ? t.bg : t.fg);
    c.setTextDatum(middle_center);
    c.drawString(bt.label, bt.x + bw / 2, by + bh / 2);
    c.setTextDatum(top_left);
  }
}

// ---------------------------------------------------------------- Choice

void ChoiceDialog::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (lv_.handle(e, 5)) { invalidate(); return; }
  if (e.nav == Nav::Enter) {
    int pick = lv_.sel;
    k.pop();
    if (cb_) cb_(pick);
  } else if (e.nav == Nav::Back) {
    k.pop();
  } else if (e.ch >= '1' && e.ch <= '9' && (e.ch - '1') < lv_.count) {
    int pick = e.ch - '1';
    k.pop();
    if (cb_) cb_(pick);
  }
}

void ChoiceDialog::render(M5Canvas& c, const Rect&) {
  const Theme& t = theme::get();
  int rows = lv_.count < 5 ? lv_.count : 5;
  Rect b = dialogBox(24 + rows * 16 + 6);
  drawFrame(c, b, title_);
  Rect list{b.x + 4, b.y + 20, b.w - 8, rows * 16};
  lv_.rowH = 16;
  ui::drawList(c, list, lv_, [&](int i, const Rect& r, bool sel) {
    ui::drawRowBg(c, r, sel);
    c.setFont(ui::small());
    c.setTextColor(sel ? t.fg : t.dim);
    char num[4];
    snprintf(num, sizeof(num), "%d", i + 1);
    c.drawString(num, r.x + 8, r.y + 4);
    c.drawString(ui::ellipsize(c, opts_[i], r.w - 30).c_str(), r.x + 20, r.y + 4);
  });
  ui::drawScrollbar(c, list, lv_);
}

// ---------------------------------------------------------------- TextInput

void TextInputDialog::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (e.nav == Nav::Enter) {
    String v = text_;
    v.trim();
    k.pop();
    if (cb_ && v.length()) cb_(v);
    return;
  }
  // Esc: so ` sozinho (sem shift) cancela; com fn vira o caractere `
  if (e.nav == Nav::Back && !e.fn) { k.pop(); return; }
  if (e.nav == Nav::Del) {
    if (text_.length()) text_.remove(text_.length() - 1);
  } else if (e.ch >= 32 && e.ch < 127 && (int)text_.length() < maxLen_) {
    text_ += e.ch;
  }
  invalidate();
}

void TextInputDialog::render(M5Canvas& c, const Rect&) {
  const Theme& t = theme::get();
  Rect b = dialogBox(64);
  drawFrame(c, b, title_);
  Rect field{b.x + 8, b.y + 22, b.w - 16, 20};
  c.fillRoundRect(field.x, field.y, field.w, field.h, 4, t.bg);
  c.setFont(ui::body());
  c.setTextColor(t.fg, t.bg);
  String shown = ui::ellipsize(c, text_, field.w - 12);
  c.drawString(shown.c_str(), field.x + 4, field.y + 2);
  if ((millis() / 450) % 2 == 0) {
    int cx = field.x + 5 + c.textWidth(shown.c_str());
    c.fillRect(cx, field.y + 3, 2, 14, t.accent);
  }
  c.setFont(ui::small());
  c.setTextColor(t.dim, t.panel);
  c.drawString("OK salvar   ` cancelar   del apagar", b.x + 8, b.y + b.h - 12);
}
