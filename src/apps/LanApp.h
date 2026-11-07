#pragma once
#include <FS.h>

#include <memory>

#include "../core/App.h"
#include "../core/Net.h"
#include "../core/Storage.h"

class WebServer;

// Arquivos pela rede: um servidor web no Cardputer com gerenciador de arquivos
// (SD e flash): enviar, baixar, renomear e apagar arquivos.
// Protegido por usuario "orbita" + PIN mostrado na tela. Funciona ligado a um
// roteador ou criando a propria rede (o celular conecta direto no Cardputer).
class LanApp : public App {
 public:
  LanApp();
  ~LanApp() override;
  const char* title() const override { return "Arquivos pela rede"; }
  void onEnter() override;
  void onExit() override;
  void onKey(const KeyEvent& e) override;
  void update(uint32_t now) override;
  void render(M5Canvas& c, const Rect& area) override;
  bool animating() const override { return !serverOn_; }  // animacao enquanto conecta

 private:
  void startServer();
  void stopServer();
  void routes();
  bool auth();
  bool argPath(const char* name, storage::Vol& vol, String& path);
  void handleInfo();
  void handleList();
  void handleDownload();
  void handleDelete();
  void handleMkdir();
  void handleRename();
  void handleUploadData();
  void handleUploadDone();

  std::unique_ptr<WebServer> server_;
  bool serverOn_ = false;
  bool weStartedWifi_ = false;
  bool askedMode_ = false;
  String pin_;
  String lastAction_;

  // envio em andamento
  File upFile_;
  bool upDenied_ = false;
  String upError_;
  size_t upBytes_ = 0;
  uint32_t upLastDraw_ = 0;
};
