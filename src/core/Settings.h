#pragma once
#include <Arduino.h>

// Ajustes do sistema, guardados em /.orbita/ajustes na flash interna (LittleFS).
struct Settings {
  uint8_t  brightness = 140;   // 0..255
  uint16_t dimSecs    = 30;    // 0 = nunca escurece
  uint8_t  accent     = 0;     // indice em theme::ACCENTS
  bool     autoboot   = false; // ao ligar, abre o GLIDE (qualquer tecla cancela)
  bool     sound      = true;  // cliques do sistema

  // Controle de apresentacao
  uint8_t  talkMinutes = 15;    // duracao do regressivo
  bool     talkCountdown = true;
  bool     pageKeys = false;    // false = setas, true = PageUp/PageDown

  void load();
  void save() const;
};
