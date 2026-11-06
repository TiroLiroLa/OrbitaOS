#include "SystemApp.h"

#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Net.h"
#include "../core/Storage.h"
#include "../core/Theme.h"

static constexpr int LINE_H = 11;

static String hex(uint32_t v) {
  char b[12];
  snprintf(b, sizeof(b), "0x%06X", (unsigned)v);
  return String(b);
}

void SystemApp::collect() {
  Kernel& k = Kernel::get();
  lines_.clear();
  auto H = [&](const String& t) { lines_.push_back({t, "", true}); };
  auto L = [&](const String& a, const String& b) { lines_.push_back({a, b, false}); };

  H(String(cfg::OS_NAME) + " " + cfg::OS_VERSION);
  L("Bootloader", k.customBootloader() ? "Orbita (RST volta)" : "PADRAO - instale o Orbita");
  L("Ultimo reset", k.resetReasonText());
  int sel = k.slots().bootSelected();
  L("Proximo esp_restart", sel < 0 ? String("sistema") : "slot " + String(sel + 1));

  H("Hardware");
  L("Chip", String(ESP.getChipModel()) + " rev " + String(ESP.getChipRevision()));
  L("CPU", String(ESP.getCpuFreqMHz()) + " MHz");
  L("Flash", storage::humanSize(ESP.getFlashChipSize()));
  L("RAM livre", storage::humanSize(ESP.getFreeHeap()));
  L("Menor RAM livre", storage::humanSize(ESP.getMinFreeHeap()));
  L("Maior bloco", storage::humanSize(ESP.getMaxAllocHeap()));
  int bat = M5Cardputer.Power.getBatteryLevel();
  L("Bateria", bat >= 0 ? String(bat) + "%" : String("?"));
  int mv = M5Cardputer.Power.getBatteryVoltage();
  if (mv > 0) L("Tensao", String(mv) + " mV");

  H("Armazenamento");
  for (int i = 0; i < storage::VOL_COUNT; i++) {
    storage::Vol v = (storage::Vol)i;
    if (storage::ok(v))
      L(storage::volName(v), storage::humanSize(storage::usedBytes(v)) + " de " +
                                 storage::humanSize(storage::totalBytes(v)));
    else
      L(storage::volName(v), "indisponivel");
  }

  H("Rede");
  L("Wi-Fi", net::stateText());
  if (net::online()) {
    L("Rede", net::ssid());
    L("IP", net::ip());
  }
  L("Redes salvas", String((int)net::known().size()));

  H("Slots de apps");
  for (int i = 0; i < k.slots().count(); i++) {
    const SlotInfo& s = k.slots().slot(i);
    String v = String("vazio");
    if (s.valid) {
      v = s.name;
      if (s.version.length()) v += " " + s.version;
    }
    L("Slot " + String(i + 1), v);
  }

  H("Particoes");
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_partition_iterator_t it =
      esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
  while (it) {
    const esp_partition_t* p = esp_partition_get(it);
    String name = String(p->label) + (running && p->address == running->address ? " *" : "");
    L(name, hex(p->address) + "  " + storage::humanSize(p->size));
    it = esp_partition_next(it);
  }
  // esp_partition_next() ja libera o iterador ao chegar no fim
  invalidate();
}

void SystemApp::onKey(const KeyEvent& e) {
  const int visible = (cfg::SCREEN_H - cfg::STATUS_H - 14) / LINE_H;
  const int maxTop = (int)lines_.size() > visible ? (int)lines_.size() - visible : 0;
  if (e.nav == Nav::Up) top_--;
  else if (e.nav == Nav::Down) top_++;
  else if (e.nav == Nav::Left) top_ -= visible;
  else if (e.nav == Nav::Right) top_ += visible;
  else if (e.nav == Nav::Back) { Kernel::get().pop(); return; }
  else if (e.ch == 'r') collect();
  if (top_ < 0) top_ = 0;
  if (top_ > maxTop) top_ = maxTop;
  invalidate();
}

void SystemApp::update(uint32_t now) {
  if (now - lastRefresh_ > 3000) {  // memoria e bateria mudam
    lastRefresh_ = now;
    collect();
  }
}

void SystemApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  c.setFont(ui::small());
  const int visible = (area.h - 14) / LINE_H;
  for (int r = 0; r < visible; r++) {
    int i = top_ + r;
    if (i >= (int)lines_.size()) break;
    const Line& l = lines_[i];
    int y = area.y + 3 + r * LINE_H;
    if (l.header) {
      c.setTextColor(t.accent);
      c.drawString(l.key.c_str(), 6, y);
      c.drawFastHLine(8 + c.textWidth(l.key.c_str()), y + 4, cfg::SCREEN_W, t.panel);
    } else {
      c.setTextColor(t.dim);
      c.drawString(l.key.c_str(), 12, y);
      c.setTextColor(t.fg);
      c.setTextDatum(top_right);
      c.drawString(ui::ellipsize(c, l.value, 130).c_str(), cfg::SCREEN_W - 8, y);
      c.setTextDatum(top_left);
    }
  }
  ui::drawHint(c, "^v rolar   r atualizar   ` voltar");
}
