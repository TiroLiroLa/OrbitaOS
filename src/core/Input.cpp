#include "Input.h"

#include <M5Cardputer.h>

#include "Config.h"

static Nav navFromChar(char c) {
  switch (c) {
    case ';': return Nav::Up;
    case '.': return Nav::Down;
    case ',': return Nav::Left;
    case '/': return Nav::Right;
    case '`': return Nav::Back;
    default:  return Nav::None;
  }
}

void Input::push(const KeyEvent& e) {
  uint8_t n = (uint8_t)((head_ + 1) % QSIZE);
  if (n == tail_) return;  // fila cheia: descarta
  queue_[head_] = e;
  head_ = n;
}

bool Input::next(KeyEvent& ev) {
  if (tail_ == head_) return false;
  ev = queue_[tail_];
  tail_ = (uint8_t)((tail_ + 1) % QSIZE);
  return true;
}

void Input::clear() {
  head_ = tail_ = 0;
  hasHeld_ = false;
}

bool Input::anyKeyDown() const { return M5Cardputer.Keyboard.isPressed() > 0; }

void Input::update() {
  auto& kb = M5Cardputer.Keyboard;
  const uint32_t now = millis();

  if (kb.isChange()) {
    lastActivity_ = now;
    hasHeld_ = false;
    if (!kb.isPressed()) return;

    Keyboard_Class::KeysState st = kb.keysState();
    KeyEvent base;
    base.fn = st.fn;
    base.shift = st.shift;
    base.ctrl = st.ctrl;
    base.alt = st.alt;
    base.opt = st.opt;

    bool emitted = false;
    auto emit = [&](const KeyEvent& e) {
      push(e);
      held_ = e;
      emitted = true;
    };

    if (st.enter) { KeyEvent e = base; e.nav = Nav::Enter; emit(e); }
    if (st.del)   { KeyEvent e = base; e.nav = Nav::Del;   emit(e); }
    if (st.tab)   { KeyEvent e = base; e.nav = Nav::Tab;   emit(e); }
    for (char c : st.word) {
      KeyEvent e = base;
      e.ch = c;
      e.nav = navFromChar(c);
      emit(e);
    }

    if (emitted) {
      hasHeld_ = true;
      heldSince_ = now;
      lastRepeat_ = now;
    }
    return;
  }

  // Sem mudanca: repete a ultima tecla se ela continua segurada
  if (hasHeld_ && kb.isPressed()) {
    if (now - heldSince_ > cfg::KEY_REPEAT_DELAY_MS &&
        now - lastRepeat_ > cfg::KEY_REPEAT_RATE_MS) {
      KeyEvent e = held_;
      e.repeat = true;
      push(e);
      lastRepeat_ = now;
      lastActivity_ = now;
    }
  } else if (!kb.isPressed()) {
    hasHeld_ = false;
  }
}
