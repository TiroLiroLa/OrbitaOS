#include "NotesApp.h"

#include <algorithm>

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"
#include "EditorApp.h"

String NotesApp::pathOf(const String& name) const {
  return storage::joinPath(cfg::DIR_NOTES, name);
}

void NotesApp::load() {
  storage::begin();  // tenta o SD de novo, caso o cartao tenha sido colocado agora
  vol_ = storage::notesVol();
  storage::ensureDirs();
  title_ = String("Notas (") + (vol_ == storage::Vol::SD ? "SD" : "flash") + ")";
  notes_.clear();

  if (storage::ok(vol_)) {
    File dir = storage::fs(vol_).open(cfg::DIR_NOTES);
    if (dir && dir.isDirectory()) {
      while (true) {
        File f = dir.openNextFile();
        if (!f) break;
        String n = storage::baseName(String(f.name()));
        bool isDir = f.isDirectory();
        uint32_t sz = (uint32_t)f.size();
        f.close();
        if (!isDir && (storage::hasExt(n, ".txt") || storage::hasExt(n, ".md")))
          notes_.push_back({n, sz});
      }
      dir.close();
    }
  }
  std::sort(notes_.begin(), notes_.end(), [](const Note& a, const Note& b) {
    String x = a.name, y = b.name;
    x.toLowerCase();
    y.toLowerCase();
    return x < y;
  });
  lv_.setCount((int)notes_.size() + 1);
  invalidate();
}

void NotesApp::newNote() {
  Kernel::get().textInput("Nome da nota", "", [this](const String& raw) {
    Kernel& k = Kernel::get();
    String name = raw;
    name.trim();
    if (!storage::hasExt(name, ".txt") && !storage::hasExt(name, ".md")) name += ".txt";
    if (!storage::validName(name)) {
      k.toast("Nome invalido");
      return;
    }
    String p = pathOf(name);
    if (storage::exists(vol_, p)) {
      k.toast("Ja existe uma nota com esse nome");
      return;
    }
    File f = storage::fs(vol_).open(p, FILE_WRITE);  // cria vazia
    if (!f) {
      k.toast("Nao deu para criar");
      return;
    }
    f.close();
    k.push(new EditorApp(vol_, p));
  });
}

void NotesApp::noteMenu(int idx) {
  const String name = notes_[idx].name;
  const String p = pathOf(name);
  Kernel::get().choose(name, {"Abrir", "Renomear", "Apagar"}, [this, name, p](int i) {
    Kernel& k = Kernel::get();
    if (i == 0) {
      k.push(new EditorApp(vol_, p));
    } else if (i == 1) {
      k.textInput("Novo nome", name, [this, p](const String& raw) {
        Kernel& k = Kernel::get();
        String nn = raw;
        nn.trim();
        if (!storage::hasExt(nn, ".txt") && !storage::hasExt(nn, ".md")) nn += ".txt";
        if (!storage::validName(nn) || !storage::renameAny(vol_, p, pathOf(nn)))
          k.toast("Nao deu para renomear");
        load();
      });
    } else if (i == 2) {
      k.confirm("Apagar nota?", name, [this, p]() {
        storage::removeAny(vol_, p);
        load();
      }, "Apagar", "Cancelar");
    }
  });
}

void NotesApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (lv_.handle(e, 6)) { invalidate(); return; }
  if (e.nav == Nav::Enter) {
    if (lv_.sel == 0) newNote();
    else k.push(new EditorApp(vol_, pathOf(notes_[lv_.sel - 1].name)));
  } else if (e.nav == Nav::Tab || e.nav == Nav::Del) {
    if (lv_.sel > 0) noteMenu(lv_.sel - 1);
  } else if (e.ch == 'n') {
    newNote();
  } else if (e.nav == Nav::Back) {
    k.pop();
  }
}

void NotesApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  if (!storage::ok(vol_)) {
    c.setFont(ui::small());
    c.setTextColor(t.warn);
    c.drawString("Sem cartao SD e sem flash interna", 10, area.y + 14);
    ui::drawHint(c, "` voltar");
    return;
  }
  Rect list{area.x, area.y + 2, area.w, area.h - 14};
  lv_.rowH = 17;
  ui::drawList(c, list, lv_, [&](int i, const Rect& r, bool sel) {
    ui::drawRowBg(c, r, sel);
    c.setFont(ui::small());
    if (i == 0) {
      c.setTextColor(t.accent);
      c.drawString("+  Nova nota", r.x + 10, r.y + 5);
      return;
    }
    const Note& n = notes_[i - 1];
    String label = n.name;
    int dot = label.lastIndexOf('.');
    if (dot > 0) label = label.substring(0, dot);
    String size = storage::humanSize(n.size);
    c.setTextColor(sel ? t.fg : t.dim);
    c.drawString(ui::ellipsize(c, label, r.w - 70).c_str(), r.x + 10, r.y + 5);
    c.setTextColor(t.dim);
    c.setTextDatum(top_right);
    c.drawString(size.c_str(), r.x + r.w - 8, r.y + 5);
    c.setTextDatum(top_left);
  });
  ui::drawScrollbar(c, list, lv_);
  ui::drawHint(c, "OK abrir   n nova   tab opcoes   ` voltar");
}
