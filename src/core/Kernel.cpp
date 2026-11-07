#include "Kernel.h"

#include <esp_flash.h>
#include <esp_system.h>
#include <math.h>

#include "../apps/HomeApp.h"
#include "Config.h"
#include "Dialogs.h"
#include "Net.h"
#include "Glide.h"
#include "Hid.h"
#include "UsbDrive.h"
#include "Storage.h"
#include "Theme.h"
#include "UI.h"

Kernel& Kernel::get() {
  static Kernel k;
  return k;
}

// ================================================================ inicio

void Kernel::begin() {
  auto m5cfg = M5.config();
  M5Cardputer.begin(m5cfg, true);  // true = teclado
  Serial.begin(115200);

  M5Cardputer.Display.setRotation(1);
  storage::beginFlash();  // os ajustes ficam na flash interna
  settings_.load();
  applyTheme();
  applyBrightness();
  M5Cardputer.Speaker.setVolume(60);

  canvas_.setColorDepth(16);
  canvas_.setPsram(false);
  if (!canvas_.createSprite(cfg::SCREEN_W, cfg::SCREEN_H)) {
    // Sem memoria para o buffer: cai para 8 bits (metade da RAM)
    canvas_.setColorDepth(8);
    canvas_.createSprite(cfg::SCREEN_W, cfg::SCREEN_H);
  }

  customBoot_ = detectCustomBootloader();
  storage::begin();
  hid::beginUsb();    // teclado USB (controle de apresentacao) - antes de iniciar o USB
  usbdrive::begin();  // registra o cartao como pendrive (desligado ate abrir o app USB)
  net::begin();

  Serial.printf("[orbita] v%s | bootloader %s | reset: %s | heap livre %u\n", cfg::OS_VERSION,
                customBoot_ ? "Orbita" : "padrao", resetReasonText(), ESP.getFreeHeap());

  bootSequence();

  push(new HomeApp());
  applyOps();
  input_.clear();
}

bool Kernel::detectCustomBootloader() {
  const size_t n = cfg::BOOT_SCAN_BYTES;
  uint8_t* buf = (uint8_t*)malloc(n);
  if (!buf) return false;
  bool found = false;
  if (esp_flash_read(nullptr, buf, 0x0, n) == ESP_OK) {
    const size_t ml = strlen(cfg::BOOT_MARK);
    for (size_t i = 0; i + ml <= n && !found; i++) {
      if (buf[i] == (uint8_t)cfg::BOOT_MARK[0] && memcmp(buf + i, cfg::BOOT_MARK, ml) == 0)
        found = true;
    }
  }
  free(buf);
  return found;
}

void Kernel::bootSequence() {
  const Theme& t = theme::get();
  auto& c = canvas_;

  // Animacao curta: um ponto em orbita ao redor do nome
  const uint32_t start = millis();
  const uint32_t dur = 700;
  while (millis() - start < dur) {
    float ph = (millis() - start) / (float)dur;
    c.fillScreen(t.bg);
    int cx = cfg::SCREEN_W / 2, cy = cfg::SCREEN_H / 2 - 6;
    c.drawEllipse(cx, cy, 70, 22, t.accentDark);
    float a = ph * 2 * (float)M_PI - (float)M_PI / 2;
    c.fillCircle(cx + (int)(cosf(a) * 70), cy + (int)(sinf(a) * 22), 4, t.accent);
    c.setFont(ui::title());
    c.setTextColor(t.fg);
    c.setTextDatum(middle_center);
    c.drawString(cfg::OS_NAME, cx, cy);
    c.setFont(ui::small());
    c.setTextColor(t.dim);
    c.drawString((String("v") + cfg::OS_VERSION).c_str(), cx, cy + 34);
    c.setTextDatum(top_left);
    c.pushSprite(&M5Cardputer.Display, 0, 0);
    delay(16);
  }

  // Abrir o GLIDE automaticamente? So ao ligar, e qualquer tecla cancela.
  if (!settings_.autoboot || !glide::present()) return;
  if (esp_reset_reason() != ESP_RST_POWERON) return;

  const uint32_t wait = 2000;
  const uint32_t t0 = millis();
  while (millis() - t0 < wait) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isPressed()) {
      // Espera soltar para a tecla nao vazar para o menu
      while (M5Cardputer.Keyboard.isPressed()) {
        M5Cardputer.update();
        delay(10);
      }
      return;
    }
    float left = (wait - (millis() - t0)) / 1000.0f;
    drawProgress("Abrindo o GLIDE",
                 "qualquer tecla fica no " + String(cfg::OS_NAME) + " (" + String(left, 1) + "s)",
                 1.0f - left * 1000.0f / wait);
    delay(30);
  }
  String err;
  glide::launch(err);  // nao retorna se der certo
}

// ================================================================ pilha

void Kernel::push(App* app) { ops_.push_back({Op::Push, app}); }
void Kernel::pop() { ops_.push_back({Op::Pop, nullptr}); }
void Kernel::replaceTop(App* app) { ops_.push_back({Op::Replace, app}); }
void Kernel::popToRoot() { ops_.push_back({Op::Root, nullptr}); }

void Kernel::applyOps() {
  bool changed = false;
  // ops_ pode crescer enquanto aplicamos (onEnter/onResume chamando push)
  for (size_t i = 0; i < ops_.size(); i++) {
    Op op = ops_[i];
    switch (op.type) {
      case Op::Push:
        stack_.emplace_back(op.app);
        op.app->onEnter();
        break;
      case Op::Pop:
        if (stack_.size() > 1) {
          stack_.back()->onExit();
          stack_.pop_back();
          stack_.back()->onResume();
        }
        break;
      case Op::Replace:
        if (!stack_.empty()) {
          stack_.back()->onExit();
          stack_.pop_back();
        }
        stack_.emplace_back(op.app);
        op.app->onEnter();
        break;
      case Op::Root:
        while (stack_.size() > 1) {
          stack_.back()->onExit();
          stack_.pop_back();
        }
        if (!stack_.empty()) stack_.back()->onResume();
        break;
    }
    changed = true;
  }
  ops_.clear();
  if (changed) forceRedraw_ = true;
}

// ================================================================ loop

void Kernel::loop() {
  M5Cardputer.update();
  input_.update();
  const uint32_t now = millis();

  KeyEvent ev;
  while (input_.next(ev)) {
    if (dimmed_) {  // a primeira tecla so acorda a tela
      dimmed_ = false;
      applyBrightness();
      forceRedraw_ = true;
      continue;
    }
    if (settings_.sound && !ev.repeat) beep();
    if (!stack_.empty()) stack_.back()->onKey(ev);
    applyOps();
  }

  net::update();
  handlePower(now);
  if (!stack_.empty()) stack_.back()->update(now);
  applyOps();

  // Precisa redesenhar?
  bool need = forceRedraw_;
  for (auto& a : stack_)
    if (a->dirty()) need = true;
  if (!stack_.empty() && stack_.back()->animating() && now - lastFrame_ >= cfg::FRAME_MS)
    need = true;
  if (toastUntil_ && now > toastUntil_) {
    toastUntil_ = 0;
    need = true;
  }
  if (need) render();

  delay(2);
}

void Kernel::handlePower(uint32_t now) {
  if (now - lastStatus_ > 5000 || lastStatus_ == 0) {
    lastStatus_ = now;
    int b = M5Cardputer.Power.getBatteryLevel();
    if (b != battery_) {
      battery_ = b;
      forceRedraw_ = true;
    }
  }
  bool awake = !stack_.empty() && stack_.back()->keepAwake();
  if (settings_.dimSecs && !dimmed_ && !awake &&
      now - input_.lastActivity() > (uint32_t)settings_.dimSecs * 1000) {
    dimmed_ = true;
    uint8_t low = settings_.brightness / 8;
    M5Cardputer.Display.setBrightness(low < 4 ? 4 : low);
  }
}

// ================================================================ desenho

void Kernel::render() {
  if (stack_.empty()) return;
  const Theme& t = theme::get();
  auto& c = canvas_;
  c.fillScreen(t.bg);

  // Acha o app "de base": o primeiro, de cima para baixo, que nao e overlay
  int base = (int)stack_.size() - 1;
  while (base > 0 && stack_[base]->overlay()) base--;
  App* baseApp = stack_[base].get();

  Rect full{0, 0, cfg::SCREEN_W, cfg::SCREEN_H};
  Rect area = baseApp->fullscreen()
                  ? full
                  : Rect{0, cfg::STATUS_H, cfg::SCREEN_W, cfg::SCREEN_H - cfg::STATUS_H};
  c.setTextDatum(top_left);
  baseApp->render(c, area);
  if (!baseApp->fullscreen()) drawStatusBar(c, baseApp->title());

  for (size_t i = base + 1; i < stack_.size(); i++) {
    c.setTextDatum(top_left);
    stack_[i]->render(c, area);
  }

  if (toastUntil_ && millis() < toastUntil_) {
    c.setFont(ui::small());
    int w = c.textWidth(toastMsg_.c_str()) + 16;
    if (w > cfg::SCREEN_W - 8) w = cfg::SCREEN_W - 8;
    int x = (cfg::SCREEN_W - w) / 2, y = cfg::SCREEN_H - 30;
    c.fillRoundRect(x, y, w, 18, 6, t.fg);
    c.setTextColor(t.bg);
    c.setTextDatum(middle_center);
    c.drawString(ui::ellipsize(c, toastMsg_, w - 10).c_str(), cfg::SCREEN_W / 2, y + 9);
    c.setTextDatum(top_left);
  }

  c.pushSprite(&M5Cardputer.Display, 0, 0);
  for (auto& a : stack_) a->clean();
  forceRedraw_ = false;
  lastFrame_ = millis();
}

void Kernel::drawStatusBar(M5Canvas& c, const char* title) {
  const Theme& t = theme::get();
  const int H = cfg::STATUS_H;
  c.fillRect(0, 0, cfg::SCREEN_W, H, t.panel);

  // Mini logo: orbita com um ponto
  c.drawEllipse(9, H / 2, 6, 3, t.accentDark);
  c.fillCircle(14, H / 2 - 1, 2, t.accent);

  c.setFont(ui::small());
  c.setTextColor(t.fg, t.panel);
  c.drawString(ui::ellipsize(c, title, 108).c_str(), 22, 3);

  // Bateria
  int bx = cfg::SCREEN_W - 22, by = 3;
  c.drawRect(bx, by, 18, 8, t.dim);
  c.fillRect(bx + 18, by + 2, 2, 4, t.dim);
  if (battery_ >= 0) {
    int fill = 16 * battery_ / 100;
    uint16_t col = battery_ <= 15 ? t.warn : t.accent;
    c.fillRect(bx + 1, by + 1, fill, 6, col);
    char pct[6];
    snprintf(pct, sizeof(pct), "%d%%", battery_);
    c.setTextColor(t.dim, t.panel);
    c.setTextDatum(top_right);
    c.drawString(pct, bx - 3, 3);
    c.setTextDatum(top_left);
  }

  // SD, Wi-Fi e aviso de bootloader (da direita para a esquerda)
  int x = cfg::SCREEN_W - 68;
  if (storage::sdOk()) {
    c.setTextColor(t.dim, t.panel);
    c.drawString("SD", x, 3);
  }
  x -= 14;
  net::State ns = net::state();
  if (ns != net::State::Off && ns != net::State::Failed) {
    uint16_t col = net::online() ? t.accent : t.dim;
    int wx = x + 5, wy = 11;
    c.fillCircle(wx, wy, 1, col);
    c.fillArc(wx, wy, 5, 4, 225, 315, col);
    c.fillArc(wx, wy, 8, 7, 225, 315, col);
  }
  x -= 22;
  if (!customBoot_) {
    c.setTextColor(t.warn, t.panel);
    c.drawString("BL!", x, 3);
  }
}

void Kernel::drawProgress(const String& title, const String& sub, float pct) {
  const Theme& t = theme::get();
  auto& c = canvas_;
  const bool indeterminate = pct < 0;  // tamanho total desconhecido
  if (pct > 1) pct = 1;
  c.fillScreen(t.bg);
  c.setTextDatum(middle_center);
  c.setFont(ui::title());
  c.setTextColor(t.fg);
  c.drawString(ui::ellipsize(c, title, cfg::SCREEN_W - 20).c_str(), cfg::SCREEN_W / 2, 44);
  c.setFont(ui::small());
  c.setTextColor(t.dim);
  c.drawString(ui::ellipsize(c, sub, cfg::SCREEN_W - 20).c_str(), cfg::SCREEN_W / 2, 66);
  int bw = cfg::SCREEN_W - 40, bx = 20, by = 82;
  c.fillRoundRect(bx, by, bw, 10, 5, t.panel);
  if (indeterminate) {
    int seg = bw / 4;
    int off = (int)((millis() / 6) % (uint32_t)(bw - seg));
    c.fillRoundRect(bx + off, by, seg, 10, 5, t.accent);
  } else {
    c.fillRoundRect(bx, by, (int)(bw * pct), 10, 5, t.accent);
    char p[8];
    snprintf(p, sizeof(p), "%d%%", (int)(pct * 100));
    c.drawString(p, cfg::SCREEN_W / 2, 104);
  }
  c.setTextDatum(top_left);
  c.pushSprite(&M5Cardputer.Display, 0, 0);
}

// ================================================================ dialogos

void Kernel::toast(const String& msg, uint32_t ms) {
  toastMsg_ = msg;
  toastUntil_ = millis() + ms;
  forceRedraw_ = true;
}

void Kernel::confirm(const String& title, const String& msg, std::function<void()> onYes,
                     const char* yes, const char* no) {
  push(new ConfirmDialog(title, msg, onYes, yes, no));
}

void Kernel::choose(const String& title, const std::vector<String>& options,
                    std::function<void(int)> onPick) {
  push(new ChoiceDialog(title, options, onPick));
}

void Kernel::textInput(const String& title, const String& initial,
                       std::function<void(const String&)> onOk, int maxLen) {
  push(new TextInputDialog(title, initial, onOk, maxLen));
}

// ================================================================ fluxos

void Kernel::launchGlide() {
  if (!glide::present()) {
    toast("GLIDE nao gravado: grave o orbita-full.bin", 3500);
    return;
  }
  auto go = [this]() {
    drawProgress("Abrindo o GLIDE", "BtnRst volta ao " + String(cfg::OS_NAME), 1.0f);
    String err;
    if (!glide::launch(err)) toast(err, 3000);
  };
  if (!customBoot_) {
    confirm("Bootloader padrao",
            "Sem o bootloader Orbita, o BtnRst NAO volta ao sistema (so regravando "
            "pelo USB). Abrir mesmo assim?",
            go, "Abrir", "Cancelar");
  } else {
    go();
  }
}

void Kernel::rebootToOS() {
  glide::selectSystem();
  delay(50);
  esp_restart();
}

// ================================================================ util

void Kernel::beep(uint16_t freq, uint16_t ms) { M5Cardputer.Speaker.tone(freq, ms); }

void Kernel::applyTheme() {
  theme::apply(settings_.accent);
  forceRedraw_ = true;
}

void Kernel::applyBrightness() { M5Cardputer.Display.setBrightness(settings_.brightness); }

const char* Kernel::resetReasonText() const {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "ligado/RST";
    case ESP_RST_EXT:      return "pino externo";
    case ESP_RST_SW:       return "software";
    case ESP_RST_PANIC:    return "erro (panic)";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep sleep";
    case ESP_RST_BROWNOUT: return "bateria fraca";
    default:               return "desconhecido";
  }
}
