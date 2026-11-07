#include "PresenterApp.h"

#include "../core/Config.h"
#include "../core/Hid.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"
#include "../core/UI.h"

// ============================================================== ciclo de vida

void PresenterApp::onEnter() {
  bleErr_ = "";
  hid::startBle(bleErr_);
  lastBattery_ = 0;
}

void PresenterApp::onExit() { hid::stopBle(); }

// ============================================================== slides

void PresenterApp::send(uint8_t usage, uint8_t mods, const char* label) {
  lastSent_ = hid::tap(usage, mods);
  lastAction_ = label;
  lastActionAt_ = millis();
  if (!lastSent_) lastAction_ = String(label) + " (sem conexao)";
  invalidate();
}

void PresenterApp::next() {
  bool page = Kernel::get().settings().pageKeys;
  send(page ? hid::key::PAGE_DOWN : hid::key::RIGHT, 0, "> proximo");
  if (lastSent_) slide_++;
  if (!running_ && accum_ == 0 && lastSent_) startTimer();  // comeca sozinho no 1o slide
}

void PresenterApp::prev() {
  bool page = Kernel::get().settings().pageKeys;
  send(page ? hid::key::PAGE_UP : hid::key::LEFT, 0, "< anterior");
  if (lastSent_ && slide_ > 1) slide_--;
}

// ============================================================== tempo

uint32_t PresenterApp::elapsedMs() const {
  return accum_ + (running_ ? millis() - startedAt_ : 0);
}

void PresenterApp::startTimer() {
  if (running_) return;
  startedAt_ = millis();
  running_ = true;
  invalidate();
}

void PresenterApp::pauseTimer() {
  if (!running_) return;
  accum_ += millis() - startedAt_;
  running_ = false;
  invalidate();
}

void PresenterApp::resetTimer() {
  running_ = false;
  accum_ = 0;
  alertStage_ = 0;
  pendingBeeps_ = 0;
  invalidate();
}

void PresenterApp::alert(int beeps) {
  pendingBeeps_ = beeps;
  nextBeepAt_ = millis();
}

void PresenterApp::checkAlerts() {
  const Settings& s = Kernel::get().settings();
  if (!s.talkCountdown) return;
  const int32_t total = (int32_t)s.talkMinutes * 60000;
  const int32_t left = total - (int32_t)elapsedMs();
  if (alertStage_ < 1 && left <= 5 * 60000 && total > 5 * 60000) { alertStage_ = 1; alert(1); }
  if (alertStage_ < 2 && left <= 60000 && total > 60000) { alertStage_ = 2; alert(2); }
  if (alertStage_ < 3 && left <= 0) { alertStage_ = 3; alert(3); }
}

void PresenterApp::update(uint32_t now) {
  if (running_) checkAlerts();

  // Bipes de alerta (nao bloqueiam)
  if (pendingBeeps_ > 0 && (int32_t)(now - nextBeepAt_) >= 0) {
    M5Cardputer.Speaker.tone(alertStage_ >= 3 ? 1400 : 2200, 160);
    pendingBeeps_--;
    nextBeepAt_ = now + 260;
  }

  // Redesenha quando o segundo muda, quando a conexao muda, ou na piscada
  uint32_t sec = elapsedMs() / 500;  // meio segundo: piscar no tempo esgotado
  bool ble = hid::bleConnected(), usb = hid::usbConnected();
  if (sec != lastSecond_ || ble != lastBle_ || usb != lastUsb_) {
    if (ble && !lastBle_) Kernel::get().toast("Bluetooth conectado");
    lastSecond_ = sec;
    lastBle_ = ble;
    lastUsb_ = usb;
    invalidate();
  }
  if (lastActionAt_ && now - lastActionAt_ > 1500) {
    lastActionAt_ = 0;
    invalidate();
  }

  if (now - lastBattery_ > 60000 || lastBattery_ == 0) {
    lastBattery_ = now;
    int b = M5Cardputer.Power.getBatteryLevel();
    if (b >= 0) hid::setBattery((uint8_t)b);
  }
}

// ============================================================== teclas

void PresenterApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  Settings& s = k.settings();

  // Ajuste de minutos aceita tecla segurada; o resto ignora a repeticao
  if (e.ch == '-' || e.ch == '_' || e.ch == '=' || e.ch == '+') {
    int d = (e.ch == '-' || e.ch == '_') ? -1 : 1;
    if (e.shift) d *= 5;
    int m = s.talkMinutes + d;
    if (m < 1) m = 1;
    if (m > 180) m = 180;
    s.talkMinutes = (uint8_t)m;
    if (!e.repeat) s.save();
    alertStage_ = 0;
    if (running_) checkAlerts();
    invalidate();
    return;
  }
  if (e.repeat) return;

  if (e.nav == Nav::Right || e.nav == Nav::Down || e.nav == Nav::Enter || e.ch == ' ') {
    next();
  } else if (e.nav == Nav::Left || e.nav == Nav::Up || e.nav == Nav::Del) {
    prev();
  } else if (e.ch == 'i') {
    send(hid::key::F5, 0, "iniciar (F5)");
    slide_ = 1;
    if (lastSent_) startTimer();
  } else if (e.ch == 'h') {
    send(hid::key::F5, hid::mod::SHIFT, "deste slide");
    if (lastSent_) startTimer();
  } else if (e.ch == 'e') {
    send(hid::key::ESC, 0, "encerrar (Esc)");
  } else if (e.ch == 'b') {
    send(hid::key::B, 0, "tela preta");
  } else if (e.ch == 'w') {
    send(hid::key::W, 0, "tela branca");
  } else if (e.ch == 't') {
    if (running_) pauseTimer();
    else startTimer();
  } else if (e.ch == 'r') {
    if (running_) {
      k.confirm("Zerar o tempo?", "O tempo esta correndo.", [this]() { resetTimer(); }, "Zerar",
                "Cancelar");
    } else {
      resetTimer();
    }
  } else if (e.ch == 'm') {
    s.talkCountdown = !s.talkCountdown;
    s.save();
    k.toast(s.talkCountdown ? "Tempo regressivo" : "Cronometro", 900);
    invalidate();
  } else if (e.ch == 'k') {
    s.pageKeys = !s.pageKeys;
    s.save();
    k.toast(s.pageKeys ? "Envia PageUp / PageDown" : "Envia setas < >", 1400);
    invalidate();
  } else if (e.ch == 's') {
    slide_ = 1;
    invalidate();
  } else if (e.nav == Nav::Back) {
    if (running_) {
      k.confirm("Sair da apresentacao?", "O tempo para e o Bluetooth desliga.",
                []() { Kernel::get().pop(); }, "Sair", "Ficar");
    } else {
      k.pop();
    }
  }
}

// ============================================================== tela

static void fmtTime(char* out, size_t n, uint32_t ms) {
  uint32_t s = ms / 1000;
  uint32_t h = s / 3600, m = (s / 60) % 60, sec = s % 60;
  if (h > 0) snprintf(out, n, "%u:%02u:%02u", (unsigned)h, (unsigned)m, (unsigned)sec);
  else snprintf(out, n, "%02u:%02u", (unsigned)m, (unsigned)sec);
}

void PresenterApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  const Settings& s = Kernel::get().settings();
  const int cx = cfg::SCREEN_W / 2;

  // ---- conexao
  c.setFont(ui::small());
  int y = area.y + 3;
  if (!hid::bleSupported() && !hid::usbSupported()) {
    c.setTextColor(t.warn);
    c.drawString("Sem Bluetooth e sem USB neste build", 6, y);
  } else if (hid::bleConnected()) {
    c.setTextColor(t.ok);
    c.drawString("Bluetooth conectado", 6, y);
  } else if (hid::bleOn()) {
    c.setTextColor(t.dim);
    c.drawString((String("Pareie: ") + hid::BLE_NAME).c_str(), 6, y);
  } else {
    c.setTextColor(t.warn);
    c.drawString(bleErr_.length() ? bleErr_.c_str() : "Bluetooth desligado", 6, y);
  }
  if (hid::usbConnected()) {
    c.setTextColor(t.ok);
    c.setTextDatum(top_right);
    c.drawString("USB", cfg::SCREEN_W - 6, y);
    c.setTextDatum(top_left);
  }

  // ---- tempo
  const uint32_t el = elapsedMs();
  const int32_t total = (int32_t)s.talkMinutes * 60000;
  const int32_t left = total - (int32_t)el;
  bool over = false;
  uint32_t shown = el;
  uint16_t col = running_ ? t.fg : t.dim;
  if (s.talkCountdown) {
    if (left >= 0) {
      shown = (uint32_t)left + 999;  // arredonda para cima: 14:59.2 mostra 15:00
      if (shown > (uint32_t)total) shown = total;
      if (left <= 60000) col = t.warn;
      else if (left <= 5 * 60000) col = theme::rgb(255, 200, 40);
      else if (running_) col = t.ok;
    } else {
      over = true;
      shown = (uint32_t)(-left);
      col = t.warn;
    }
  }
  char buf[16];
  fmtTime(buf, sizeof(buf), shown);
  bool blinkOff = over && running_ && ((millis() / 500) % 2);

  const int ty = area.y + 44;
  if (!blinkOff) {
    c.setFont(&fonts::Font7);
    c.setTextColor(col);
    c.setTextDatum(middle_center);
    c.drawString(buf, cx, ty);
    if (over) {
      int w = c.textWidth(buf);
      c.setFont(ui::title());
      c.drawString("+", cx - w / 2 - 10, ty - 2);
    }
    c.setTextDatum(top_left);
  }

  // Barra de progresso do regressivo
  if (s.talkCountdown) {
    int bx = 20, bw = cfg::SCREEN_W - 40, by = ty + 28;
    float f = total > 0 ? (float)el / total : 0;
    if (f > 1) f = 1;
    c.fillRect(bx, by, bw, 3, t.panel);
    c.fillRect(bx, by, (int)(bw * f), 3, col);
  }

  // Modo e estado
  c.setFont(ui::small());
  c.setTextColor(t.dim);
  c.setTextDatum(top_center);
  String mode = s.talkCountdown ? "Regressivo " + String(s.talkMinutes) + " min" : String("Cronometro");
  if (!running_) mode += el ? "  -  pausado" : "  -  t para comecar";
  c.drawString(mode.c_str(), cx, ty + 35);
  c.setTextDatum(top_left);

  // ---- slide e ultima acao
  int by2 = area.y + area.h - 26;
  c.setFont(ui::title());
  c.setTextColor(t.accent);
  c.drawString(("Slide " + String(slide_)).c_str(), 8, by2 - 2);
  if (lastActionAt_) {
    c.setFont(ui::small());
    c.setTextColor(lastSent_ ? t.ok : t.warn);
    c.setTextDatum(top_right);
    c.drawString(lastAction_.c_str(), cfg::SCREEN_W - 8, by2 + 2);
    c.setTextDatum(top_left);
  } else {
    c.setFont(ui::small());
    c.setTextColor(t.dim);
    c.setTextDatum(top_right);
    c.drawString(s.pageKeys ? "teclas: PageUp/Down" : "teclas: setas", cfg::SCREEN_W - 8,
                 by2 + 2);
    c.setTextDatum(top_left);
  }

  ui::drawHint(c, "/ prox  , volta  i inicia  b preto  t tempo  -= min");
}
