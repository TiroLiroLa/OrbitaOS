#include "KvFile.h"

#include "Storage.h"

static String clean(const String& s) {
  String o;
  o.reserve(s.length());
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\t' || c == '\n' || c == '\r') continue;  // separadores nao entram
    o += c;
  }
  return o;
}

bool KvFile::load() {
  items_.clear();
  if (!storage::flashOk()) return false;
  fs::FS& f = storage::fs(storage::Vol::Flash);
  File file = f.open(path_, FILE_READ);
  if (!file) return false;
  String line;
  while (file.available()) {
    char c = (char)file.read();
    if (c == '\n') {
      int tab = line.indexOf('\t');
      if (tab > 0) items_.push_back({line.substring(0, tab), line.substring(tab + 1)});
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
  int tab = line.indexOf('\t');
  if (tab > 0) items_.push_back({line.substring(0, tab), line.substring(tab + 1)});
  file.close();
  return true;
}

bool KvFile::save() const {
  if (!storage::flashOk()) return false;
  fs::FS& f = storage::fs(storage::Vol::Flash);
  String dir = storage::parentPath(path_);
  if (!f.exists(dir)) f.mkdir(dir);
  String tmp = String(path_) + ".tmp";
  File file = f.open(tmp, FILE_WRITE);
  if (!file) return false;
  for (const auto& it : items_) {
    file.print(it.key);
    file.print('\t');
    file.print(it.value);
    file.print('\n');
  }
  file.close();
  f.remove(path_);
  return f.rename(tmp, path_);
}

String KvFile::get(const String& key, const String& def) const {
  for (const auto& it : items_)
    if (it.key == key) return it.value;
  return def;
}

long KvFile::getInt(const String& key, long def) const {
  String v = get(key, "");
  return v.length() ? v.toInt() : def;
}

bool KvFile::getBool(const String& key, bool def) const {
  String v = get(key, "");
  return v.length() ? v == "1" : def;
}

void KvFile::set(const String& key, const String& value) {
  String k = clean(key), v = clean(value);
  for (auto& it : items_) {
    if (it.key == k) {
      it.value = v;
      return;
    }
  }
  items_.push_back({k, v});
}

void KvFile::remove(const String& key) {
  for (size_t i = 0; i < items_.size(); i++) {
    if (items_[i].key == key) {
      items_.erase(items_.begin() + i);
      return;
    }
  }
}
