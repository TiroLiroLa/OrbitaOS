#include "WifiApp.h"

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"

static const char* FIXED_LABELS[] = {"Procurar redes", "Criar rede propria (sem roteador)",
                                     "Desligar Wi-Fi"};

void WifiApp::onEnter() {
  if (net::state() == net::State::Off || net::state() == net::State::Failed) net::startScan();
  rebuild();
}

void WifiApp::rebuild() {
  list_ = net::results();
  // Redes conhecidas que nao apareceram na busca tambem ficam na lista
  for (const auto& k : net::known()) {
    bool found = false;
    for (const auto& r : list_)
      if (r.ssid == k.ssid) { found = true; break; }
    if (!found) list_.push_back({k.ssid, -100, false, true});
  }
  lv_.setCount(FIXED_COUNT + (int)list_.size());
  invalidate();
}

void WifiApp::connectTo(const net::ScanResult& r) {
  Kernel& k = Kernel::get();
  for (const auto& kn : net::known()) {
    if (kn.ssid == r.ssid) {
      net::connect(kn.ssid, kn.pass);
      return;
    }
  }
  if (r.open) {
    net::connect(r.ssid, "");
    return;
  }
  String ssid = r.ssid;
  k.textInput("Senha de " + ssid, "", [ssid](const String& pw) { net::connect(ssid, pw); }, 63);
}

void WifiApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (lv_.handle(e, 6)) { invalidate(); return; }
  if (e.nav == Nav::Back) { k.pop(); return; }

  int sel = lv_.sel;
  if (e.nav == Nav::Enter) {
    if (sel == Scan) net::startScan();
    else if (sel == AP) net::startAP();
    else if (sel == Off) net::stop();
    else connectTo(list_[sel - FIXED_COUNT]);
    rebuild();
  } else if ((e.nav == Nav::Tab || e.nav == Nav::Del) && sel >= FIXED_COUNT) {
    const net::ScanResult r = list_[sel - FIXED_COUNT];
    if (!r.known) return;
    k.confirm("Esquecer rede?", r.ssid, [this, r]() {
      net::forget(r.ssid);
      rebuild();
    }, "Esquecer", "Cancelar");
  }
}

void WifiApp::update(uint32_t now) {
  if (now - lastPoll_ < 300) return;
  lastPoll_ = now;
  bool sc = net::scanning();
  net::State st = net::state();
  if (st != lastState_ || sc != wasScanning_) {
    if (lastState_ == net::State::Connecting && st == net::State::Connected)
      Kernel::get().toast("Conectado a " + net::ssid());
    if (lastState_ == net::State::Connecting && st == net::State::Failed)
      Kernel::get().toast("Falhou: " + net::lastError(), 3000);
    lastState_ = st;
    wasScanning_ = sc;
    rebuild();
  }
}

static void drawSignal(M5Canvas& c, int x, int y, int rssi, uint16_t on, uint16_t off) {
  int bars = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : rssi > -100 ? 1 : 0;
  for (int b = 0; b < 4; b++) {
    int h = 2 + b * 2;
    c.fillRect(x + b * 3, y + 8 - h, 2, h, b < bars ? on : off);
  }
}

void WifiApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();

  // Linha de estado
  c.setFont(ui::small());
  c.setTextColor(net::online() ? t.ok : t.dim);
  String st = String("Estado: ") + net::stateText();
  if (net::ssid().length()) st += "  " + net::ssid();
  if (net::scanning()) st = "Procurando redes...";
  c.drawString(ui::ellipsize(c, st, area.w - 12).c_str(), 6, area.y + 3);
  if (net::online()) {
    c.setTextColor(t.dim);
    String ipl = "IP " + net::ip();
    if (net::state() == net::State::AccessPoint) ipl += "  senha " + net::apPassword();
    c.drawString(ipl.c_str(), 6, area.y + 13);
  }

  Rect list{area.x, area.y + 24, area.w, area.h - 36};
  lv_.rowH = 15;
  ui::drawList(c, list, lv_, [&](int i, const Rect& r, bool sel) {
    ui::drawRowBg(c, r, sel);
    c.setFont(ui::small());
    if (i < FIXED_COUNT) {
      c.setTextColor(t.accent);
      c.drawString(FIXED_LABELS[i], r.x + 10, r.y + 4);
      return;
    }
    const net::ScanResult& n = list_[i - FIXED_COUNT];
    bool current = net::state() == net::State::Connected && n.ssid == net::ssid();
    c.setTextColor(current ? t.ok : (sel ? t.fg : t.dim));
    c.drawString(ui::ellipsize(c, n.ssid, r.w - 70).c_str(), r.x + 10, r.y + 4);
    c.setTextColor(t.dim);
    String tag = n.known ? "salva" : (n.open ? "aberta" : "");
    c.setTextDatum(top_right);
    c.drawString(tag.c_str(), r.x + r.w - 24, r.y + 4);
    c.setTextDatum(top_left);
    if (n.rssi > -100) drawSignal(c, r.x + r.w - 20, r.y + 3, n.rssi, t.accent, t.panel);
  });
  ui::drawScrollbar(c, list, lv_);
  ui::drawHint(c, "OK conectar   tab esquecer   ` voltar");
}
