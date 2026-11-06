#include "HomeApp.h"

#include <math.h>

#include <algorithm>

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"
#include "../core/Storage.h"
#include "FilesApp.h"
#include "FirmwareApp.h"
#include "LanApp.h"
#include "NotesApp.h"
#include "SettingsApp.h"
#include "SystemApp.h"
#include "UsbApp.h"
#include "WifiApp.h"

static ui::Icon iconForName(const String& name) {
  String n = name;
  n.toLowerCase();
  if (n.indexOf("hazke") >= 0) return ui::Icon::Planet;
  if (n.indexOf("glide") >= 0 || n.indexOf("synth") >= 0) return ui::Icon::Wave;
  return ui::Icon::Generic;
}

void HomeApp::rebuild() {
  Kernel& k = Kernel::get();
  SlotManager& sm = k.slots();
  items_.clear();

  // 1) Firmwares instalados nos slots: abrem direto (troca de firmware)
  int installed = 0;
  for (int i = 0; i < sm.count(); i++) {
    const SlotInfo& s = sm.slot(i);
    if (!s.valid) continue;
    installed++;
    Item it;
    it.slot = i;
    it.label = s.name;
    it.sub = "Slot " + String(i + 1) + (s.version.length() ? String("  ") + s.version : String());
    it.icon = iconForName(s.name);
    it.letter = s.name.length() ? (char)toupper(s.name[0]) : '?';
    it.open = [i]() { Kernel::get().launchSlot(i); };
    items_.push_back(it);
  }

  // 2) Apps do proprio Orbita
  items_.push_back({"Apps", installed ? "Trocar e gerenciar firmwares" : "Instale o Hazke e o GLIDE",
                    ui::Icon::Grid, 0, -1, []() { Kernel::get().push(new FirmwareApp()); }});
  items_.push_back({"Notas", "Bloco de notas", ui::Icon::Note, 0, -1,
                    []() { Kernel::get().push(new NotesApp()); }});
  items_.push_back({"Arquivos", "Cartao SD e flash interna", ui::Icon::Folder, 0, -1, []() {
                      storage::Vol v = storage::sdOk() ? storage::Vol::SD : storage::Vol::Flash;
                      Kernel::get().push(new FilesApp(v, "/"));
                    }});
  items_.push_back({"Rede", "Arquivos pelo navegador", ui::Icon::Globe, 0, -1,
                    []() { Kernel::get().push(new LanApp()); }});
  items_.push_back({"USB", "Cartao SD como pendrive", ui::Icon::Usb, 0, -1,
                    []() { Kernel::get().push(new UsbApp()); }});
  items_.push_back({"Wi-Fi", "Redes salvas", ui::Icon::Wifi, 0, -1,
                    []() { Kernel::get().push(new WifiApp()); }});
  items_.push_back({"Ajustes", "Tela, cores, sons, boot", ui::Icon::Gear, 0, -1,
                    []() { Kernel::get().push(new SettingsApp()); }});
  items_.push_back({"Sistema", "Memoria, particoes, bateria", ui::Icon::Chip, 0, -1,
                    []() { Kernel::get().push(new SystemApp()); }});

  if (sel_ >= (int)items_.size()) sel_ = (int)items_.size() - 1;
  if (sel_ < 0) sel_ = 0;
  pos_ = (float)sel_;
  invalidate();
}

void HomeApp::slotMenu(int slot) {
  (void)slot;
  Kernel::get().push(new FirmwareApp());
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
  } else if (e.nav == Nav::Tab || e.ch == 'o') {
    if (items_[sel_].slot >= 0) slotMenu(items_[sel_].slot);
    return;
  } else if (e.nav == Nav::Del) {
    int s = items_[sel_].slot;
    if (s >= 0 && Kernel::get().slots().slot(s).valid) slotMenu(s);
    return;
  } else if (e.ch >= '1' && e.ch <= '9') {
    // Atalho: numero = abre o app do slot
    int s = e.ch - '1';
    if (s < Kernel::get().slots().count()) Kernel::get().launchSlot(s);
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
    if (it.slot >= 0 && it.icon != ui::Icon::Empty) {
      // numero do atalho no canto
      char num[2] = {(char)('1' + it.slot), 0};
      c.setFont(ui::small());
      c.setTextColor(focused ? t.accent : t.dim);
      c.drawString(num, x - size / 2 - 2, y - size / 2 - 2);
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

  ui::drawHint(c, cur.slot >= 0 ? "<> navegar  OK abrir  tab opcoes  1-9 atalho"
                                : "<> navegar   OK abrir");
}
