#pragma once
#include <M5Cardputer.h>

#include <functional>
#include <memory>
#include <vector>

#include "App.h"
#include "Input.h"
#include "Settings.h"
#include "Storage.h"

// O "nucleo" do Orbita: inicializa o hardware, mantem a pilha de apps,
// distribui teclas, desenha a tela e cuida de energia.
class Kernel {
 public:
  static Kernel& get();

  void begin();
  void loop();

  // Pilha de apps. As operacoes sao aplicadas depois do evento atual,
  // entao um app pode chamar pop() de dentro do proprio onKey com seguranca.
  void push(App* app);          // o Kernel passa a ser o dono do ponteiro
  void pop();
  void replaceTop(App* app);
  void popToRoot();

  // Dialogos prontos (overlays)
  void toast(const String& msg, uint32_t ms = 1800);
  void confirm(const String& title, const String& msg, std::function<void()> onYes,
               const char* yes = "Sim", const char* no = "Nao");
  void choose(const String& title, const std::vector<String>& options,
              std::function<void(int)> onPick);
  void textInput(const String& title, const String& initial,
                 std::function<void(const String&)> onOk, int maxLen = 32);

  // Fluxos do sistema
  void rebootToOS();

  // Desenho direto na tela (para tarefas que bloqueiam, ex.: gravar flash)
  void drawProgress(const String& title, const String& sub, float pct);

  void beep(uint16_t freq = 2600, uint16_t ms = 8);
  void applyTheme();
  void applyBrightness();
  void requestRedraw() { forceRedraw_ = true; }

  Input& input() { return input_; }
  Settings& settings() { return settings_; }
  const char* resetReasonText() const;

 private:
  Kernel() = default;
  struct Op {
    enum Type { Push, Pop, Replace, Root } type;
    App* app;
  };

  void applyOps();
  void render();
  void drawStatusBar(M5Canvas& c, const char* title);
  void handlePower(uint32_t now);
  void bootSequence();

  std::vector<std::unique_ptr<App>> stack_;
  std::vector<Op> ops_;
  M5Canvas canvas_;
  Input input_;
  Settings settings_;

  bool forceRedraw_ = true;
  bool dimmed_ = false;
  uint32_t lastFrame_ = 0;
  uint32_t lastStatus_ = 0;
  int battery_ = -1;

  String toastMsg_;
  uint32_t toastUntil_ = 0;
};
