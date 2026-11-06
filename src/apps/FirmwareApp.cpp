#include "FirmwareApp.h"

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Storage.h"
#include "../core/Theme.h"
#include "FilesApp.h"

void FirmwareApp::onEnter() {
  Kernel& k = Kernel::get();
  k.slots().refreshAll();
  lv_.setCount(k.slots().count() + 1);
  lv_.rowH = 30;
  invalidate();
}

static void openInstaller() {
  Kernel::get().push(new FilesApp(storage::Vol::SD, cfg::DIR_APPS, true));
}

void FirmwareApp::slotMenu(int slot) {
  Kernel& k = Kernel::get();
  const SlotInfo& s = k.slots().slot(slot);
  if (!s.valid) {
    openInstaller();
    return;
  }
  const String name = s.name;
  k.choose("Slot " + String(slot + 1) + ": " + name,
           {"Abrir (troca o firmware)", "Substituir por outro .bin", "Renomear",
            "Apagar do slot"},
           [this, slot, name](int opt) {
             Kernel& k = Kernel::get();
             switch (opt) {
               case 0:
                 k.launchSlot(slot);
                 break;
               case 1:
                 openInstaller();
                 break;
               case 2:
                 k.textInput("Novo nome", name, [this, slot](const String& v) {
                   Kernel::get().slots().rename(slot, v);
                   invalidate();
                 });
                 break;
               case 3:
                 k.confirm("Apagar " + name + "?",
                           "O firmware sai do slot. Saves no cartao SD continuam.",
                           [this, slot]() {
                             String err;
                             Kernel& k = Kernel::get();
                             if (k.slots().erase(slot, err)) k.toast("Slot liberado");
                             else k.toast("Erro: " + err, 3000);
                             onEnter();
                           },
                           "Apagar", "Cancelar");
                 break;
             }
           });
}

void FirmwareApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (lv_.handle(e, 3)) { invalidate(); return; }
  int n = k.slots().count();
  if (e.nav == Nav::Back) {
    k.pop();
  } else if (e.nav == Nav::Enter) {
    if (lv_.sel >= n) openInstaller();
    else if (k.slots().slot(lv_.sel).valid) k.launchSlot(lv_.sel);
    else openInstaller();
  } else if (e.nav == Nav::Tab || e.nav == Nav::Del || e.ch == 'o') {
    if (lv_.sel < n) slotMenu(lv_.sel);
  } else if (e.ch >= '1' && e.ch <= '9' && e.ch - '1' < n) {
    k.launchSlot(e.ch - '1');
  }
}

void FirmwareApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  Kernel& k = Kernel::get();
  const Settings& st = k.settings();
  Rect list{area.x, area.y + 2, area.w, area.h - 14};
  ui::drawList(c, list, lv_, [&](int i, const Rect& r, bool sel) {
    ui::drawRowBg(c, r, sel);
    c.setFont(ui::small());
    if (i >= k.slots().count()) {
      c.setTextColor(t.accent);
      c.drawString("+  Instalar app do cartao SD", r.x + 10, r.y + 11);
      return;
    }
    const SlotInfo& s = k.slots().slot(i);
    char num[4];
    snprintf(num, sizeof(num), "%d", i + 1);
    c.setFont(ui::title());
    c.setTextColor(sel ? t.accent : t.dim);
    c.drawString(num, r.x + 10, r.y + 7);
    c.setFont(ui::small());
    c.setTextColor(s.valid ? (sel ? t.fg : t.dim) : t.dim);
    c.drawString(s.valid ? ui::ellipsize(c, s.name, 120).c_str() : "(vazio)", r.x + 28, r.y + 5);
    c.setTextColor(t.dim);
    String sub = storage::humanSize(s.part->size) + " de espaco";
    if (s.valid && s.version.length()) sub = "v" + s.version + "  " + sub;
    c.drawString(ui::ellipsize(c, sub, r.w - 40).c_str(), r.x + 28, r.y + 16);
    if (s.valid && st.autoboot && st.lastSlot == i) {
      c.setTextColor(t.ok);
      c.setTextDatum(top_right);
      c.drawString("abre ao ligar", r.x + r.w - 10, r.y + 5);
      c.setTextDatum(top_left);
    }
  });
  ui::drawScrollbar(c, list, lv_);
  ui::drawHint(c, "OK abrir   tab opcoes   1-9 abrir slot");
}
