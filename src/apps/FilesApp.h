#pragma once
#include <vector>

#include "../core/App.h"
#include "../core/Storage.h"
#include "../core/UI.h"

// Gerenciador de arquivos local: cartao SD e flash interna.
// Abrir texto, copiar/recortar/colar (inclusive entre volumes),
// renomear, nova pasta, novo arquivo, apagar (pastas inteiras tambem).
class FilesApp : public App {
 public:
  FilesApp(storage::Vol vol, const String& startDir) : vol_(vol), path_(startDir) {}
  const char* title() const override { return title_.c_str(); }
  void onEnter() override { load(); }
  void onResume() override { load(); }
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  struct Entry {
    String name;
    bool dir;
    uint32_t size;
  };
  struct Clipboard {
    bool active = false;
    bool cut = false;
    storage::Vol vol = storage::Vol::SD;
    String path;
  };

  void load();
  void openSelected();
  void openFile(const Entry& en);
  void menu();
  void paste();
  void switchVolume();
  String fullPath(const Entry& en) const { return storage::joinPath(path_, en.name); }

  storage::Vol vol_;
  String path_;
  String title_;
  std::vector<Entry> entries_;
  ui::ListView lv_;
  bool truncated_ = false;

  static Clipboard clip_;  // compartilhado entre janelas do gerenciador
};
