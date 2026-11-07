#include "HomeApp.h"

#include <math.h>

#include <algorithm>

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"
#include "../core/Storage.h"
#include "FilesApp.h"
#include "NotesApp.h"
#include "PresenterApp.h"
#include "SettingsApp.h"
#include "SystemApp.h"

void HomeApp::rebuild() {
  items_.clear();
  items_.push_back({"Apresentacao", "Passar slides + cronometro", ui::Icon::Slides, 0, 'a',
                    []() { Kernel::get().push(new PresenterApp()); }});
  items_.push_back({"Notas", "Bloco de notas", ui::Icon::Note, 0, 'n',
                    []() { Kernel::get().push(new NotesApp()); }});
  items_.push_back({"Arquivos", "Cartao SD e flash interna", ui::Icon::Folder, 0, 'f', []() {
                      storage::Vol v = storage::sdOk() ? storage::Vol::SD : storage::Vol::Flash;
                      Kernel::get().push(new FilesApp(v, "/"));
                    }});
  items_.push_back({"Ajustes", "Tela, cores e sons", ui::Icon::Gear, 0, 'j',
                    []() { Kernel::get().push(new SettingsApp()); }});
  items_.push_back({"Sistema", "Memoria, particoes, bateria", ui::Icon::Chip, 0, 's',
                    []() { Kernel::get().push(new SystemApp()); }});

  if (sel_ >= (int)items_.size()) sel_ = (int)items_.size() - 1;
  if (sel_ < 0) sel_ = 0;
  pos_ = (float)sel_;
  invalidate();
}

void HomeApp::onKey(const KeyEvent& e) {
  const int n = (int)items_.size();
  if (n == 0) return;

  if (e.nav == Nav::Left || e.nav == Nav::Up) {
    sel_ = (sel_ - 1 + n) % n;
  } else if (e.nav == Nav::Right || e.nav == Nav::Down) {
    sel_ = (sel_ + 1) % n;
  } else if (e.nav == Nav::Enter) {
    if (items_[sel_].open) items_[sel_].open();
    return;
  } else if (e.ch) {
    // Atalho: letra do app (a = Apresentacao, n = Notas...)
    for (const Item& it : items_) {
      if (it.hotkey && it.hotkey == e.ch) {
        if (it.open) it.open();
        return;
      }
    }
    return;
  } else {
    return;
  }
  animating_ = true;
  invalidate();
}

void HomeApp::update(uint32_t now) {
  if (!animating_ || now - lastStep_ < 16) return;
  lastStep_ = now;
  float d = sel_ - pos_;
  // Ao dar a volta (ultimo -> primeiro), salta direto
  if (fabsf(d) > 2.5f) pos_ = (float)sel_;
  else pos_ += d * 0.35f;
  if (fabsf(sel_ - pos_) < 0.02f) {
    pos_ = (float)sel_;
    animating_ = false;
  }
  invalidate();
}

void HomeApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  const int cx = cfg::SCREEN_W / 2;
  const int cy = area.y + 40;

  // A "orbita" de fundo
  c.drawEllipse(cx, cy + 4, 150, 26, t.panel);
  c.drawEllipse(cx, cy + 4, 151, 27, t.panel);

  // Desenha de tras pra frente: os mais distantes primeiro
  std::vector<int> order;
  for (int k = 0; k < (int)items_.size(); k++)
    if (fabsf(k - pos_) < 2.3f) order.push_back(k);
  std::sort(order.begin(), order.end(),
            [this](int a, int b) { return fabsf(a - pos_) > fabsf(b - pos_); });

  for (int k : order) {
    const Item& it = items_[k];
    float d = k - pos_;
    float ad = fabsf(d);
    float scale = 1.0f - (ad > 1.5f ? 1.5f : ad) * 0.3f;
    int size = (int)(54 * scale);
    int x = cx + (int)(d * 76);
    int y = cy + (int)(d * d * 7);
    bool focused = ad < 0.5f;

    uint16_t col = focused ? t.accent : t.dim;
    if (focused) {
      int bs = size + 14;
      c.fillRoundRect(x - bs / 2, y - bs / 2, bs, bs, 12, t.accentDark);
      c.drawRoundRect(x - bs / 2, y - bs / 2, bs, bs, 12, t.accent);
      ui::drawIcon(c, it.icon, x, y, size, col, t.accentDark, it.letter);
    } else {
      c.fillRoundRect(x - size / 2 - 5, y - size / 2 - 5, size + 10, size + 10, 10, t.panel);
      ui::drawIcon(c, it.icon, x, y, size, col, t.panel, it.letter);
    }
    if (it.hotkey) {  // letra do atalho no canto
      char hk[2] = {it.hotkey, 0};
      c.setFont(ui::small());
      c.setTextColor(focused ? t.accent : t.dim);
      c.drawString(hk, x - size / 2 - 2, y - size / 2 - 2);
    }
  }

  // Nome e descricao do item focado
  const Item& cur = items_[sel_];
  c.setTextDatum(middle_center);
  c.setFont(ui::title());
  c.setTextColor(t.fg);
  c.drawString(ui::ellipsize(c, cur.label, cfg::SCREEN_W - 16).c_str(), cx, area.y + 85);
  c.setFont(ui::small());
  c.setTextColor(t.dim);
  c.drawString(ui::ellipsize(c, cur.sub, cfg::SCREEN_W - 16).c_str(), cx, area.y + 101);
  c.setTextDatum(top_left);

  ui::drawHint(c, "<> navegar   OK abrir   letra = atalho");
}
