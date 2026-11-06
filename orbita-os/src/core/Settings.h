#pragma once
#include <Arduino.h>

// Ajustes do sistema, guardados na NVS (namespace "orbita").
struct Settings {
  uint8_t  brightness = 140;   // 0..255
  uint16_t dimSecs    = 30;    // 0 = nunca escurece
  uint8_t  accent     = 0;     // indice em theme::ACCENTS
  bool     autoboot   = false; // ao ligar, abre o ultimo app (qualquer tecla cancela)
  bool     sound      = true;  // cliques do sistema
  int8_t   lastSlot   = -1;    // ultimo app aberto

  void load();
  void save() const;
};
