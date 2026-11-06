#include "Settings.h"

#include <Preferences.h>

static const char* NS = "orbita";

void Settings::load() {
  Preferences p;
  if (!p.begin(NS, true)) return;  // primeira vez: fica com os padroes
  brightness = p.getUChar("bright", brightness);
  dimSecs    = p.getUShort("dim", dimSecs);
  accent     = p.getUChar("accent", accent);
  autoboot   = p.getBool("autoboot", autoboot);
  sound      = p.getBool("sound", sound);
  lastSlot   = p.getChar("last", lastSlot);
  p.end();
}

void Settings::save() const {
  Preferences p;
  if (!p.begin(NS, false)) return;
  p.putUChar("bright", brightness);
  p.putUShort("dim", dimSecs);
  p.putUChar("accent", accent);
  p.putBool("autoboot", autoboot);
  p.putBool("sound", sound);
  p.putChar("last", lastSlot);
  p.end();
}
