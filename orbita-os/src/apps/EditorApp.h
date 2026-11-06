#pragma once
#include <string>
#include <vector>

#include "../core/App.h"
#include "../core/Storage.h"

// Editor de texto simples (tambem serve como visualizador somente-leitura).
//
// Teclas: letras digitam normalmente (inclusive ; , . /)
//   fn + ; , . /   move o cursor (cima, esquerda, baixo, direita)
//   fn + del       apaga para a frente      del  apaga para tras
//   ctrl + s       salva                    `    sai (pergunta se houver mudancas)
//   fn + `         digita o caractere `
class EditorApp : public App {
 public:
  EditorApp(storage::Vol vol, const String& path, bool readOnly = false);
  const char* title() const override { return title_.c_str(); }
  void onEnter() override;
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;
  bool animating() const override { return !readOnly_; }  // cursor piscando

  static constexpr size_t MAX_BYTES = 32 * 1024;

 private:
  struct VLine {
    size_t start, len;
  };
  void load();
  bool save();
  void layout();
  int lineOfCursor() const;
  void moveVert(int dir);
  void insert(char ch);
  void exitAsk();

  storage::Vol vol_;
  String path_, title_;
  bool readOnly_;
  bool modified_ = false;
  bool truncated_ = false;
  std::string text_;
  std::vector<VLine> lines_;
  size_t cur_ = 0;
  int top_ = 0;
  int cols_ = 38;
  int rows_ = 10;
};
