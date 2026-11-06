#pragma once
#include <M5Cardputer.h>

#include "Input.h"

struct Rect {
  int x, y, w, h;
};

// Base de todo app nativo do Orbita.
//
// O Kernel mantem uma pilha de apps. So o app do topo recebe teclas.
// Cada quadro o Kernel limpa o canvas, chama render() do topo (e do app
// de baixo, se o topo for um overlay como uma caixa de dialogo), desenha a
// barra de status e envia tudo para a tela de uma vez (sem piscar).
class App {
 public:
  virtual ~App() = default;

  virtual const char* title() const = 0;

  virtual void onEnter() {}          // ao entrar na pilha
  virtual void onResume() {}         // quando o app de cima sai
  virtual void onExit() {}           // ao sair da pilha
  virtual void onKey(const KeyEvent& e) = 0;
  virtual void update(uint32_t nowMs) { (void)nowMs; }
  virtual void render(M5Canvas& c, const Rect& area) = 0;

  virtual bool overlay() const { return false; }     // desenha por cima do app anterior
  virtual bool fullscreen() const { return false; }  // esconde a barra de status
  virtual bool animating() const { return false; }   // pede quadros continuos

  void invalidate() { dirty_ = true; }
  bool dirty() const { return dirty_; }
  void clean() { dirty_ = false; }

 private:
  bool dirty_ = true;
};
