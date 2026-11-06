#pragma once
#include <functional>
#include <vector>

#include "App.h"
#include "UI.h"

// Caixa Sim/Nao
class ConfirmDialog : public App {
 public:
  ConfirmDialog(const String& title, const String& msg, std::function<void()> onYes,
                const char* yes, const char* no)
      : title_(title), msg_(msg), onYes_(onYes), yes_(yes), no_(no) {}
  const char* title() const override { return title_.c_str(); }
  bool overlay() const override { return true; }
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  String title_, msg_;
  std::function<void()> onYes_;
  const char* yes_;
  const char* no_;
  bool yesSelected_ = false;  // padrao seguro: "Nao"
};

// Lista de opcoes
class ChoiceDialog : public App {
 public:
  ChoiceDialog(const String& title, const std::vector<String>& opts, std::function<void(int)> cb)
      : title_(title), opts_(opts), cb_(cb) {
    lv_.setCount((int)opts_.size());
  }
  const char* title() const override { return title_.c_str(); }
  bool overlay() const override { return true; }
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  String title_;
  std::vector<String> opts_;
  std::function<void(int)> cb_;
  ui::ListView lv_;
};

// Campo de texto de uma linha
class TextInputDialog : public App {
 public:
  TextInputDialog(const String& title, const String& initial,
                  std::function<void(const String&)> cb, int maxLen = 32)
      : title_(title), text_(initial), cb_(cb), maxLen_(maxLen) {}
  const char* title() const override { return title_.c_str(); }
  bool overlay() const override { return true; }
  bool animating() const override { return true; }  // cursor piscando
  void onKey(const KeyEvent& e) override;
  void render(M5Canvas& c, const Rect& area) override;

 private:
  String title_, text_;
  std::function<void(const String&)> cb_;
  int maxLen_;
};
