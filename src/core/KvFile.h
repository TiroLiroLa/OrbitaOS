#pragma once
#include <Arduino.h>

#include <vector>

// Arquivo simples de "chave<TAB>valor" por linha, guardado na flash interna
// (LittleFS, em /.orbita). Usado no lugar da NVS para os ajustes do Orbita,
// porque a NVS e compartilhada com o GLIDE e ele pode limpa-la.
class KvFile {
 public:
  struct Item {
    String key, value;
  };

  explicit KvFile(const char* path) : path_(path) {}
  bool load();          // false se o arquivo nao existe (fica vazio)
  bool save() const;    // grava em .tmp e troca, para nao corromper se faltar energia

  String get(const String& key, const String& def = "") const;
  long getInt(const String& key, long def) const;
  bool getBool(const String& key, bool def) const;
  void set(const String& key, const String& value);
  void setInt(const String& key, long v) { set(key, String(v)); }
  void setBool(const String& key, bool v) { set(key, v ? "1" : "0"); }
  void remove(const String& key);
  void clear() { items_.clear(); }
  const std::vector<Item>& items() const { return items_; }

 private:
  const char* path_;
  std::vector<Item> items_;
};
