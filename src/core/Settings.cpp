#include "Settings.h"

#include "Config.h"
#include "KvFile.h"

void Settings::load() {
  KvFile kv(cfg::FILE_SETTINGS);
  if (!kv.load()) return;  // primeira vez: fica com os padroes
  brightness    = (uint8_t)kv.getInt("brilho", brightness);
  dimSecs       = (uint16_t)kv.getInt("escurecer", dimSecs);
  accent        = (uint8_t)kv.getInt("cor", accent);
  autoboot      = kv.getBool("abrir_glide", autoboot);
  sound         = kv.getBool("sons", sound);
  talkMinutes   = (uint8_t)kv.getInt("apres_min", talkMinutes);
  talkCountdown = kv.getBool("apres_regressivo", talkCountdown);
  pageKeys      = kv.getBool("apres_page", pageKeys);
  if (talkMinutes < 1) talkMinutes = 1;
}

void Settings::save() const {
  KvFile kv(cfg::FILE_SETTINGS);
  kv.setInt("brilho", brightness);
  kv.setInt("escurecer", dimSecs);
  kv.setInt("cor", accent);
  kv.setBool("abrir_glide", autoboot);
  kv.setBool("sons", sound);
  kv.setInt("apres_min", talkMinutes);
  kv.setBool("apres_regressivo", talkCountdown);
  kv.setBool("apres_page", pageKeys);
  kv.save();
}
