#pragma once
#include <vector>

#include "../core/App.h"
#include "../core/Storage.h"
#include "../core/UI.h"

// Lista de notas (.txt em /orbita/notas no SD, ou na flash interna sem cartao)
class NotesApp : public App {
 public:
  const char* title() const override { return title_.c_str(); }
  void onEnter() override { load(); }
  void onResume() override { load(); }
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  struct Note {
    String name;
    uint32_t size;
  };
  void load();
  void newNote();
  void noteMenu(int idx);
  String pathOf(const String& name) const;

  storage::Vol vol_ = storage::Vol::SD;
  String title_ = "Notas";
  std::vector<Note> notes_;
  ui::ListView lv_;  // item 0 = "+ Nova nota"
};
