#include "EditorApp.h"

#include "../core/Config.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"
#include "../core/UI.h"

static constexpr int CHAR_W = 6;   // Font0
static constexpr int LINE_H = 10;

EditorApp::EditorApp(storage::Vol vol, const String& path, bool readOnly)
    : vol_(vol), path_(path), readOnly_(readOnly) {
  title_ = storage::baseName(path) + (readOnly ? " (leitura)" : "");
}

void EditorApp::onEnter() {
  load();
  layout();
}

void EditorApp::load() {
  text_.clear();
  if (!storage::ok(vol_)) return;
  File f = storage::fs(vol_).open(path_, FILE_READ);
  if (!f) return;  // arquivo novo
  size_t size = f.size();
  if (size > MAX_BYTES) {
    truncated_ = true;
    readOnly_ = true;
    size = MAX_BYTES;
    title_ = storage::baseName(path_) + " (32 KB, leitura)";
  }
  text_.resize(size);
  size_t got = f.read((uint8_t*)&text_[0], size);
  text_.resize(got);
  f.close();
  // Normaliza quebras de linha do Windows e troca caracteres de controle
  std::string clean;
  clean.reserve(text_.size());
  for (char ch : text_) {
    if (ch == '\r') continue;
    if (ch == '\t') { clean += "  "; continue; }
    if ((uint8_t)ch < 32 && ch != '\n') ch = '?';
    clean += ch;
  }
  text_.swap(clean);
  cur_ = 0;
}

bool EditorApp::save() {
  if (readOnly_) return false;
  if (!storage::ok(vol_)) return false;
  File f = storage::fs(vol_).open(path_, FILE_WRITE);
  if (!f) return false;
  size_t w = f.write((const uint8_t*)text_.data(), text_.size());
  f.close();
  if (w != text_.size()) return false;
  modified_ = false;
  return true;
}

// Quebra o texto em linhas visuais (por palavra, com corte forcado se preciso)
void EditorApp::layout() {
  lines_.clear();
  const size_t n = text_.size();
  size_t i = 0;
  while (true) {
    size_t eol = text_.find('\n', i);
    if (eol == std::string::npos) eol = n;
    size_t s = i;
    while (eol - s > (size_t)cols_) {
      size_t cut = s + cols_;
      size_t sp = text_.rfind(' ', cut);
      if (sp != std::string::npos && sp > s) cut = sp + 1;  // quebra depois do espaco
      lines_.push_back({s, cut - s});
      s = cut;
    }
    lines_.push_back({s, eol - s});
    if (eol >= n) break;
    i = eol + 1;
  }
}

int EditorApp::lineOfCursor() const {
  int l = 0;
  for (int k = 0; k < (int)lines_.size(); k++) {
    if (lines_[k].start <= cur_) l = k;
    else break;
  }
  return l;
}

void EditorApp::moveVert(int dir) {
  int l = lineOfCursor();
  int t = l + dir;
  if (t < 0 || t >= (int)lines_.size()) return;
  size_t col = cur_ - lines_[l].start;
  size_t len = lines_[t].len;
  cur_ = lines_[t].start + (col < len ? col : len);
}

void EditorApp::insert(char ch) {
  if (text_.size() >= MAX_BYTES) {
    Kernel::get().toast("Limite de 32 KB");
    return;
  }
  text_.insert(text_.begin() + cur_, ch);
  cur_++;
  modified_ = true;
}

void EditorApp::exitAsk() {
  Kernel& k = Kernel::get();
  if (!modified_) {
    k.pop();
    return;
  }
  k.choose("Salvar " + storage::baseName(path_) + "?",
           {"Salvar e sair", "Sair sem salvar", "Continuar editando"}, [this](int i) {
             Kernel& k = Kernel::get();
             if (i == 0) {
               if (save()) k.pop();
               else k.toast("Erro ao salvar!", 3000);
             } else if (i == 1) {
               k.pop();
             }
           });
}

void EditorApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  const bool arrow = e.nav == Nav::Up || e.nav == Nav::Down || e.nav == Nav::Left ||
                     e.nav == Nav::Right;

  if (readOnly_) {  // no modo leitura as setas funcionam sem fn
    int maxTop = (int)lines_.size() - rows_;
    if (maxTop < 0) maxTop = 0;
    if (e.nav == Nav::Up) top_--;
    else if (e.nav == Nav::Down) top_++;
    else if (e.nav == Nav::Left) top_ -= rows_ - 1;
    else if (e.nav == Nav::Right) top_ += rows_ - 1;
    else if (e.nav == Nav::Back || e.nav == Nav::Enter) { k.pop(); return; }
    if (top_ < 0) top_ = 0;
    if (top_ > maxTop) top_ = maxTop;
    invalidate();
    return;
  }

  if (e.fn && arrow) {
    if (e.nav == Nav::Left && cur_ > 0) cur_--;
    else if (e.nav == Nav::Right && cur_ < text_.size()) cur_++;
    else if (e.nav == Nav::Up) moveVert(-1);
    else if (e.nav == Nav::Down) moveVert(+1);
  } else if (e.nav == Nav::Back && !e.fn) {
    exitAsk();
    return;
  } else if (e.ctrl && (e.ch == 's' || e.ch == 'S')) {
    bool ok = save();
    k.toast(ok ? "Salvo" : "Erro ao salvar!", ok ? 1000 : 3000);
  } else if (e.nav == Nav::Enter) {
    insert('\n');
  } else if (e.nav == Nav::Del) {
    if (e.fn) {
      if (cur_ < text_.size()) { text_.erase(cur_, 1); modified_ = true; }
    } else if (cur_ > 0) {
      text_.erase(cur_ - 1, 1);
      cur_--;
      modified_ = true;
    }
  } else if (e.nav == Nav::Tab) {
    insert(' ');
    insert(' ');
  } else if (e.ch >= 32 && e.ch < 127) {
    insert(e.ch);
  } else {
    return;
  }
  layout();
  invalidate();
}

void EditorApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  cols_ = (area.w - 10) / CHAR_W;
  rows_ = (area.h - 14) / LINE_H;
  c.setFont(ui::small());

  if (!readOnly_) {  // mantem o cursor visivel
    int l = lineOfCursor();
    if (l < top_) top_ = l;
    if (l >= top_ + rows_) top_ = l - rows_ + 1;
  }

  const int x0 = area.x + 5, y0 = area.y + 3;
  int curLine = lineOfCursor();
  for (int r = 0; r < rows_; r++) {
    int li = top_ + r;
    if (li >= (int)lines_.size()) break;
    const VLine& L = lines_[li];
    std::string s = text_.substr(L.start, L.len);
    c.setTextColor(t.fg);
    c.drawString(s.c_str(), x0, y0 + r * LINE_H);
    if (!readOnly_ && li == curLine && (millis() / 500) % 2 == 0) {
      int cx = x0 + (int)(cur_ - L.start) * CHAR_W;
      c.fillRect(cx, y0 + r * LINE_H - 1, 2, LINE_H, t.accent);
    }
  }

  // Barra de rolagem
  if ((int)lines_.size() > rows_) {
    int h = area.h - 14;
    int bh = h * rows_ / (int)lines_.size();
    if (bh < 6) bh = 6;
    int by = area.y + (h - bh) * top_ / ((int)lines_.size() - rows_);
    c.fillRect(area.w - 3, area.y, 2, h, t.panel);
    c.fillRect(area.w - 3, by, 2, bh, t.accent);
  }

  char info[64];
  if (readOnly_) {
    snprintf(info, sizeof(info), "setas rolar  ` sair   %d/%d", top_ + 1, (int)lines_.size());
  } else {
    int l = curLine;
    snprintf(info, sizeof(info), "fn+setas  ctrl+s salvar  ` sair %s L%d", modified_ ? "*" : "",
             l + 1);
  }
  ui::drawHint(c, info);
}
