#include "Settings.h"

#include <Preferences.h>

static const char* NS = "orbita";

void Settings::load() {
  Preferences p;
  if (!p.begin(NS, true)) return;  // primeira vez: fica com os padroes
  brightness    = p.getUChar("bright", brightness);
  dimSecs       = p.getUShort("dim", dimSecs);
  accent        = p.getUChar("accent", accent);
  sound         = p.getBool("sound", sound);
  talkMinutes   = p.getUChar("tmin", talkMinutes);
  talkCountdown = p.getBool("tdown", talkCountdown);
  pageKeys      = p.getBool("tpage", pageKeys);
  p.end();
  if (talkMinutes < 1) talkMinutes = 1;
}

void Settings::save() const {
  Preferences p;
  if (!p.begin(NS, false)) return;
  p.putUChar("bright", brightness);
  p.putUShort("dim", dimSecs);
  p.putUChar("accent", accent);
  p.putBool("sound", sound);
  p.putUChar("tmin", talkMinutes);
  p.putBool("tdown", talkCountdown);
  p.putBool("tpage", pageKeys);
  p.end();
}
