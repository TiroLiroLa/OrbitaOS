#pragma once
#include <Arduino.h>

// Navegacao: no Cardputer as setas ficam em ; , . /  e o Esc e a tecla ` .
enum class Nav : uint8_t { None, Up, Down, Left, Right, Enter, Back, Del, Tab };

struct KeyEvent {
  Nav  nav = Nav::None;
  char ch  = 0;  // caractere digitado (0 se nao houver). Ex.: ';' tem ch=';' e nav=Up
  bool fn = false, shift = false, ctrl = false, alt = false, opt = false;
  bool repeat = false;  // gerado por tecla segurada

  bool is(char c) const { return ch == c; }
};

// Transforma o estado do teclado do M5Cardputer (Cardputer e ADV) em uma fila
// de eventos, com repeticao para teclas seguradas.
class Input {
 public:
  void update();                 // chamar uma vez por loop, depois de M5Cardputer.update()
  bool next(KeyEvent& ev);       // tira o proximo evento da fila
  void clear();
  uint32_t lastActivity() const { return lastActivity_; }
  bool anyKeyDown() const;

 private:
  void push(const KeyEvent& e);

  static constexpr uint8_t QSIZE = 16;
  KeyEvent queue_[QSIZE];
  uint8_t head_ = 0, tail_ = 0;

  KeyEvent held_;
  bool hasHeld_ = false;
  uint32_t heldSince_ = 0, lastRepeat_ = 0;
  uint32_t lastActivity_ = 0;
};
