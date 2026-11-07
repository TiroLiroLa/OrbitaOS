#pragma once
#include "../core/App.h"

// Controle de apresentacao: o Cardputer vira um "passador de slides" (teclado
// Bluetooth ou USB) com cronometro / tempo regressivo na tela.
class PresenterApp : public App {
 public:
  const char* title() const override { return "Apresentacao"; }
  void onEnter() override;
  void onExit() override;
  void onKey(const KeyEvent& e) override;
  void update(uint32_t now) override;
  void render(M5Canvas& c, const Rect& area) override;
  bool keepAwake() const override { return true; }  // o tempo precisa ficar visivel

 private:
  // Slides
  void send(uint8_t usage, uint8_t mods, const char* label);
  void next();
  void prev();

  // Tempo
  uint32_t elapsedMs() const;
  void startTimer();
  void pauseTimer();
  void resetTimer();
  void checkAlerts();
  void alert(int beeps);

  bool running_ = false;
  uint32_t startedAt_ = 0;   // millis() quando o tempo comecou a correr
  uint32_t accum_ = 0;       // tempo acumulado antes da ultima pausa
  int alertStage_ = 0;       // 0 nenhum, 1 = faltam 5 min, 2 = falta 1 min, 3 = acabou
  int pendingBeeps_ = 0;
  uint32_t nextBeepAt_ = 0;

  int slide_ = 1;
  String lastAction_;
  uint32_t lastActionAt_ = 0;
  bool lastSent_ = false;

  String bleErr_;
  bool lastBle_ = false, lastUsb_ = false;
  uint32_t lastSecond_ = 0;
  uint32_t lastBattery_ = 0;
};
