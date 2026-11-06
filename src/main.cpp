// Orbita OS para o M5Stack Cardputer ADV
#include <Arduino.h>

#include "core/Kernel.h"

void setup() { Kernel::get().begin(); }

void loop() { Kernel::get().loop(); }
