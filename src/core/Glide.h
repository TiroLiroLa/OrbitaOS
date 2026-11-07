#pragma once
#include <Arduino.h>
#include <esp_partition.h>

// O GLIDE original fica gravado na particao "glide" (ota_0). Abrir = apontar o
// otadata para ela e reiniciar; o bootloader Orbita traz de volta no BtnRst.
namespace glide {

bool present();               // ha uma imagem valida gravada na particao
const esp_partition_t* partition();
uint32_t imageSize();         // tamanho aproximado (0 se ausente)
bool launch(String& err);     // nao retorna se der certo
void selectSystem();          // o proximo esp_restart() cai no Orbita
bool bootSelected();          // o otadata aponta para o GLIDE agora

constexpr const char* NOTICE =
    "GLIDE (C) 2026 Charles Tobin (CHARL3X) - PolyForm Noncommercial 1.0.0 - "
    "github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV";

}  // namespace glide
