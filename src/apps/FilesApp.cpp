#include "FilesApp.h"

#include <algorithm>
#include <functional>

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"
#include "EditorApp.h"

static constexpr size_t MAX_ENTRIES = 200;

FilesApp::Clipboard FilesApp::clip_;

static const char* volShort(storage::Vol v) { return v == storage::Vol::SD ? "SD" : "Flash"; }

void FilesApp::load() {
  entries_.clear();
  truncated_ = false;
  if (vol_ == storage::Vol::SD) storage::begin();  // tenta montar se o cartao entrou agora

  if (storage::ok(vol_) && !storage::exists(vol_, path_)) path_ = "/";
  title_ = String(volShort(vol_)) + ":" + path_;

  if (!storage::ok(vol_)) {
    lv_.setCount(0);
    invalidate();
    return;
  }

  File dir = storage::fs(vol_).open(path_);
  if (dir && dir.isDirectory()) {
    while (true) {
      File f = dir.openNextFile();
      if (!f) break;
      String name = storage::baseName(String(f.name()));
      bool isDir = f.isDirectory();
      uint32_t size = isDir ? 0 : (uint32_t)f.size();
      f.close();
      if (name.length() == 0 || name[0] == '.') continue;  // ocultos
      if (entries_.size() >= MAX_ENTRIES) { truncated_ = true; break; }
      entries_.push_back({name, isDir, size});
    }
    dir.close();
  }

  std::sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
    if (a.dir != b.dir) return a.dir;
    String x = a.name, y = b.name;
    x.toLowerCase();
    y.toLowerCase();
    return x < y;
  });
  lv_.setCount((int)entries_.size());
  invalidate();
}

void FilesApp::switchVolume() {
  vol_ = vol_ == storage::Vol::SD ? storage::Vol::Flash : storage::Vol::SD;
  path_ = "/";
  lv_.sel = lv_.top = 0;
  load();
  Kernel::get().toast(storage::volName(vol_), 900);
}

void FilesApp::openFile(const Entry& en) {
  Kernel& k = Kernel::get();
  String full = fullPath(en);
  if (storage::isTextName(en.name)) {
    k.push(new EditorApp(vol_, full));
  } else {
    // Formato desconhecido: oferece ver como texto
    k.confirm("Abrir como texto?", en.name + " (" + storage::humanSize(en.size) + ")",
              [this, full]() { Kernel::get().push(new EditorApp(vol_, full, true)); }, "Abrir",
              "Cancelar");
  }
}

void FilesApp::openSelected() {
  if (entries_.empty()) return;
  const Entry& en = entries_[lv_.sel];
  if (en.dir) {
    path_ = fullPath(en);
    lv_.sel = lv_.top = 0;
    load();
  } else {
    openFile(en);
  }
}

void FilesApp::paste() {
  Kernel& k = Kernel::get();
  if (!clip_.active) { k.toast("Nada para colar"); return; }
  String name = storage::baseName(clip_.path);
  String dst = storage::joinPath(path_, name);
  if (clip_.vol == vol_ && clip_.path == dst) { k.toast("Ja esta aqui"); return; }

  String err;
  uint32_t lastDraw = 0;
  const char* verb = clip_.cut ? "Movendo " : "Copiando ";
  auto progress = [&](const String& item, size_t done, size_t total) {
    uint32_t now = millis();
    if (now - lastDraw < 150 && done < total) return;
    lastDraw = now;
    k.drawProgress(String(verb) + name, item, total ? (float)done / total : 1.0f);
  };
  k.drawProgress(String(verb) + name, "", 0);
  bool ok = clip_.cut ? storage::moveAny(clip_.vol, clip_.path, vol_, dst, err, progress)
                      : storage::copyAny(clip_.vol, clip_.path, vol_, dst, err, progress);
  k.input().clear();
  k.requestRedraw();
  if (!ok) {
    k.toast("Falhou: " + err, 3500);
  } else {
    k.toast(clip_.cut ? "Movido" : "Copiado");
    if (clip_.cut) clip_.active = false;  // depois de mover, a origem nao existe mais
  }
  load();
}

void FilesApp::menu() {
  Kernel& k = Kernel::get();
  std::vector<String> opts;
  std::vector<std::function<void()>> acts;
  auto add = [&](const String& label, std::function<void()> fn) {
    opts.push_back(label);
    acts.push_back(fn);
  };

  const bool hasSel = !entries_.empty();
  if (hasSel) {
    const Entry en = entries_[lv_.sel];
    const String full = fullPath(en);
    if (!en.dir) {
      add("Editar como texto", [this, full]() { Kernel::get().push(new EditorApp(vol_, full)); });
      add("Ver como texto", [this, full]() {
        Kernel::get().push(new EditorApp(vol_, full, true));
      });
    }
    add("Copiar", [this, full]() {
      clip_ = {true, false, vol_, full};
      Kernel::get().toast("Copiado: escolha a pasta e cole");
    });
    add("Recortar", [this, full]() {
      clip_ = {true, true, vol_, full};
      Kernel::get().toast("Recortado: escolha a pasta e cole");
    });
    add("Renomear", [this, en, full]() {
      Kernel::get().textInput("Novo nome", en.name, [this, full](const String& raw) {
        String nn = raw;
        nn.trim();
        Kernel& k = Kernel::get();
        if (!storage::validName(nn)) { k.toast("Nome invalido"); return; }
        if (!storage::renameAny(vol_, full, storage::joinPath(path_, nn)))
          k.toast("Nao deu para renomear");
        load();
      }, 64);
    });
    add("Apagar", [this, en, full]() {
      String msg = en.dir ? "A pasta " + en.name + " e TUDO dentro dela." : en.name;
      Kernel::get().confirm("Apagar?", msg, [this, full]() {
        Kernel& k = Kernel::get();
        k.drawProgress("Apagando", storage::baseName(full), 0.5f);
        bool ok = storage::removeAny(vol_, full);
        k.requestRedraw();
        k.toast(ok ? "Apagado" : "Nao apagou tudo");
        if (clip_.active && clip_.vol == vol_ && clip_.path.startsWith(full)) clip_.active = false;
        load();
      }, "Apagar", "Cancelar");
    });
  }
  if (clip_.active) {
    add("Colar aqui: " + storage::baseName(clip_.path), [this]() { paste(); });
  }
  add("Nova pasta", [this]() {
    Kernel::get().textInput("Nome da pasta", "", [this](const String& raw) {
      String nn = raw;
      nn.trim();
      Kernel& k = Kernel::get();
      if (!storage::validName(nn)) { k.toast("Nome invalido"); return; }
      if (!storage::makeDir(vol_, storage::joinPath(path_, nn))) k.toast("Nao criou a pasta");
      load();
    }, 64);
  });
  add("Novo arquivo de texto", [this]() {
    Kernel::get().textInput("Nome do arquivo", "novo.txt", [this](const String& raw) {
      String nn = raw;
      nn.trim();
      Kernel& k = Kernel::get();
      if (!storage::validName(nn)) { k.toast("Nome invalido"); return; }
      String p = storage::joinPath(path_, nn);
      if (storage::exists(vol_, p)) { k.toast("Ja existe"); return; }
      k.push(new EditorApp(vol_, p));
    }, 64);
  });
  add(String("Ir para ") + (vol_ == storage::Vol::SD ? "flash interna" : "cartao SD"),
      [this]() { switchVolume(); });
  add("Informacoes do volume", [this]() {
    Kernel::get().toast(storage::humanSize(storage::usedBytes(vol_)) + " usados de " +
                            storage::humanSize(storage::totalBytes(vol_)),
                        3000);
  });

  String title = hasSel ? entries_[lv_.sel].name : String("Pasta ") + path_;
  k.choose(title, opts, [acts](int i) {
    if (i >= 0 && i < (int)acts.size()) acts[i]();
  });
}

void FilesApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (lv_.handle(e, 6)) { invalidate(); return; }

  if (e.nav == Nav::Enter) {
    if (!storage::ok(vol_)) { load(); return; }  // tenta montar de novo
    openSelected();
  } else if (e.nav == Nav::Back) {
    if (path_ == "/") {
      k.pop();
    } else {
      String leaving = storage::baseName(path_);
      path_ = storage::parentPath(path_);
      load();
      for (size_t i = 0; i < entries_.size(); i++)  // volta a selecao para a pasta de onde saimos
        if (entries_[i].name == leaving) { lv_.sel = (int)i; break; }
    }
  } else if (e.nav == Nav::Tab || e.nav == Nav::Del || e.ch == 'm') {
    if (storage::ok(vol_)) menu();
  } else if (e.ch == 'c' && !entries_.empty()) {
    clip_ = {true, false, vol_, fullPath(entries_[lv_.sel])};
    k.toast("Copiado");
  } else if (e.ch == 'x' && !entries_.empty()) {
    clip_ = {true, true, vol_, fullPath(entries_[lv_.sel])};
    k.toast("Recortado");
  } else if (e.ch == 'v') {
    paste();
  } else if (e.ch == 'f') {
    switchVolume();
  } else if (e.ch == 'r') {
    load();
    k.toast("Atualizado", 700);
  }
}

void FilesApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  Rect list{area.x, area.y + 2, area.w, area.h - 14};

  if (!storage::ok(vol_)) {
    c.setFont(ui::body());
    c.setTextColor(t.fg);
    c.setTextDatum(middle_center);
    const char* msg = vol_ == storage::Vol::SD
                          ? "Sem cartao SD"
                          : "Sem flash interna (instale pelo Launcher)";
    c.drawString(msg, cfg::SCREEN_W / 2, area.y + 40);
    c.setFont(ui::small());
    c.setTextColor(t.dim);
    c.drawString("Enter tenta de novo   f troca de volume", cfg::SCREEN_W / 2, area.y + 62);
    c.setTextDatum(top_left);
    ui::drawHint(c, "OK tentar   f volume   ` voltar");
    return;
  }

  if (entries_.empty()) {
    c.setFont(ui::small());
    c.setTextColor(t.dim);
    c.drawString("Pasta vazia", 10, area.y + 14);
    ui::drawHint(c, "tab menu   f volume   ` voltar");
    return;
  }

  lv_.rowH = 17;
  ui::drawList(c, list, lv_, [&](int i, const Rect& r, bool sel) {
    const Entry& en = entries_[i];
    ui::drawRowBg(c, r, sel);
    c.setFont(ui::small());
    bool txt = storage::isTextName(en.name);
    bool inClip = clip_.active && clip_.vol == vol_ && clip_.path == fullPath(en);
    c.setTextColor(en.dir ? t.accent : (txt ? t.fg : t.dim));
    c.drawString(en.dir ? "[]" : (txt ? "==" : " -"), r.x + 8, r.y + 5);
    c.setTextColor(inClip ? t.accent : (sel ? t.fg : t.dim));
    String size = en.dir ? String("") : storage::humanSize(en.size);
    int sw = c.textWidth(size.c_str());
    c.drawString(ui::ellipsize(c, en.name, r.w - 40 - sw).c_str(), r.x + 26, r.y + 5);
    c.setTextColor(t.dim);
    c.setTextDatum(top_right);
    c.drawString(size.c_str(), r.x + r.w - 8, r.y + 5);
    c.setTextDatum(top_left);
  });
  ui::drawScrollbar(c, list, lv_);

  if (truncated_) ui::drawHint(c, "Mostrando os 200 primeiros");
  else if (clip_.active) ui::drawHint(c, "v colar aqui   tab menu   ` voltar");
  else ui::drawHint(c, "OK abrir  tab menu  c/x/v copiar  f volume");
}
