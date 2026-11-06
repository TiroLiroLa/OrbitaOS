#include "UsbApp.h"

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Storage.h"
#include "../core/Theme.h"
#include "../core/UI.h"
#include "../core/UsbDrive.h"

void UsbApp::onEnter() {
  err_ = "";
  ok_ = usbdrive::start(err_);
}

void UsbApp::onExit() {
  if (ok_) usbdrive::stop();
  ok_ = false;
}

void UsbApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (e.nav != Nav::Back && e.nav != Nav::Enter) return;
  if (ok_ && usbdrive::hostSeen() && !usbdrive::ejected()) {
    k.confirm("Sair sem ejetar?",
              "Se o computador ainda estiver gravando, arquivos podem corromper. Ejete "
              "o disco no computador primeiro.",
              []() { Kernel::get().pop(); }, "Sair", "Esperar");
  } else {
    k.pop();
  }
}

void UsbApp::update(uint32_t now) {
  if (now - lastPoll_ > 500) {  // contadores de leitura/gravacao
    lastPoll_ = now;
    invalidate();
  }
}

void UsbApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  const int cx = 46, cy = area.y + 50;
  ui::drawIcon(c, ui::Icon::Usb, cx, cy, 60, ok_ ? t.accent : t.dim, t.bg);

  const int x = 92;
  int y = area.y + 12;
  c.setFont(ui::title());
  if (!ok_) {
    c.setTextColor(t.warn);
    c.drawString("Indisponivel", x, y);
    c.setFont(ui::small());
    c.setTextColor(t.dim);
    ui::drawWrapped(c, err_, x, y + 24, cfg::SCREEN_W - x - 6, 10);
    ui::drawHint(c, "` voltar");
    return;
  }

  bool ej = usbdrive::ejected();
  bool seen = usbdrive::hostSeen();
  c.setTextColor(ej ? t.ok : t.fg);
  c.drawString(ej ? "Ejetado" : (seen ? "Conectado" : "Aguardando"), x, y);
  c.setFont(ui::small());
  c.setTextColor(t.dim);
  String msg = ej ? "Pode sair com seguranca."
                  : (seen ? "O cartao SD esta no computador. Ejete la antes de sair."
                          : "Ligue o cabo USB-C no computador. O cartao aparece como disco.");
  ui::drawWrapped(c, msg, x, y + 22, cfg::SCREEN_W - x - 6, 10);

  c.setTextColor(t.fg);
  c.drawString(("Lido:    " + storage::humanSize(usbdrive::bytesRead())).c_str(), x, area.y + 82);
  c.drawString(("Gravado: " + storage::humanSize(usbdrive::bytesWritten())).c_str(), x,
               area.y + 94);
  ui::drawHint(c, ej ? "` voltar" : "ejete no computador, depois `");
}
