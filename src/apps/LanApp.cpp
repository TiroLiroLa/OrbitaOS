#include "LanApp.h"

#include <ESPmDNS.h>
#include <WebServer.h>
#include <esp_random.h>

#include "../core/Config.h"
#include "../core/Glide.h"
#include "../core/Kernel.h"
#include "../core/Theme.h"
#include "../core/UI.h"
#include "WebUi.h"
#include "WifiApp.h"

// ------------------------------------------------------------ util

static String jsonEsc(const String& s) {
  String o;
  o.reserve(s.length() + 4);
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 32) o += ' ';
    else o += c;
  }
  return o;
}

static const char* mimeFor(const String& name) {
  if (storage::hasExt(name, ".htm") || storage::hasExt(name, ".html")) return "text/html";
  if (storage::hasExt(name, ".txt") || storage::hasExt(name, ".md") ||
      storage::hasExt(name, ".log") || storage::hasExt(name, ".csv"))
    return "text/plain; charset=utf-8";
  if (storage::hasExt(name, ".json")) return "application/json";
  if (storage::hasExt(name, ".png")) return "image/png";
  if (storage::hasExt(name, ".jpg") || storage::hasExt(name, ".jpeg")) return "image/jpeg";
  if (storage::hasExt(name, ".bmp")) return "image/bmp";
  if (storage::hasExt(name, ".wav")) return "audio/wav";
  return "application/octet-stream";
}

// ------------------------------------------------------------ ciclo de vida

LanApp::LanApp() = default;
LanApp::~LanApp() { stopServer(); }

void LanApp::onEnter() {
  char p[8];
  snprintf(p, sizeof(p), "%06lu", (unsigned long)(esp_random() % 1000000UL));
  pin_ = p;

  if (net::online()) return;  // update() liga o servidor
  if (net::connectKnown()) {
    weStartedWifi_ = true;
    return;
  }
  askedMode_ = true;
  Kernel::get().choose("Sem Wi-Fi. Como conectar?",
                       {"Escolher uma rede Wi-Fi", "Criar rede propria (celular conecta direto)"},
                       [this](int i) {
                         weStartedWifi_ = true;
                         if (i == 0) Kernel::get().push(new WifiApp());
                         else net::startAP();
                       });
}

void LanApp::onExit() {
  stopServer();
  if (weStartedWifi_) net::stop();  // economiza bateria
}

void LanApp::startServer() {
  if (serverOn_) return;
  server_.reset(new WebServer(80));
  routes();
  server_->begin();
  MDNS.begin(cfg::HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  serverOn_ = true;
  lastAction_ = "Servidor ligado";
  invalidate();
}

void LanApp::stopServer() {
  if (!serverOn_) return;
  if (upFile_) upFile_.close();
  server_->stop();
  server_.reset();
  MDNS.end();
  serverOn_ = false;
}

void LanApp::update(uint32_t now) {
  if (!serverOn_ && net::online()) startServer();
  if (serverOn_ && !net::online()) {
    stopServer();
    lastAction_ = "Rede caiu";
    invalidate();
  }
  if (serverOn_) server_->handleClient();

  (void)now;
}

void LanApp::onKey(const KeyEvent& e) {
  Kernel& k = Kernel::get();
  if (e.nav == Nav::Back) {
    k.pop();
  } else if (e.ch == 'w') {
    weStartedWifi_ = true;
    k.push(new WifiApp());
  } else if (e.ch == 'a') {
    weStartedWifi_ = true;
    stopServer();
    net::startAP();
    invalidate();
  } else if (e.ch == 'p') {  // novo PIN
    char p[8];
    snprintf(p, sizeof(p), "%06lu", (unsigned long)(esp_random() % 1000000UL));
    pin_ = p;
    invalidate();
  }
}

// ------------------------------------------------------------ HTTP

bool LanApp::auth() {
  if (server_->authenticate(cfg::WEB_USER, pin_.c_str())) return true;
  server_->requestAuthentication(BASIC_AUTH, "Orbita", "Use o PIN que aparece no Cardputer");
  return false;
}

bool LanApp::argPath(const char* name, storage::Vol& vol, String& path) {
  if (!storage::volFromKey(server_->arg("vol"), vol)) {
    server_->send(400, "text/plain", "Volume invalido");
    return false;
  }
  path = server_->arg(name);
  if (path.length() == 0) path = "/";
  if (!storage::safePath(path) || path.indexOf("/.") >= 0) {  // /.orbita = arquivos do sistema
    server_->send(400, "text/plain", "Caminho invalido");
    return false;
  }
  if (!storage::ok(vol)) {
    server_->send(503, "text/plain", "Volume indisponivel");
    return false;
  }
  return true;
}

void LanApp::routes() {
  WebServer& s = *server_;
  s.on("/", HTTP_GET, [this]() {
    if (!auth()) return;
    server_->send(200, "text/html", WEB_PAGE);
  });
  s.on("/api/info", HTTP_GET, [this]() { handleInfo(); });
  s.on("/api/list", HTTP_GET, [this]() { handleList(); });
  s.on("/api/download", HTTP_GET, [this]() { handleDownload(); });
  s.on("/api/delete", HTTP_POST, [this]() { handleDelete(); });
  s.on("/api/mkdir", HTTP_POST, [this]() { handleMkdir(); });
  s.on("/api/rename", HTTP_POST, [this]() { handleRename(); });
  s.on("/api/upload", HTTP_POST, [this]() { handleUploadDone(); }, [this]() { handleUploadData(); });
  s.onNotFound([this]() { server_->send(404, "text/plain", "Nao encontrado"); });
}

void LanApp::handleInfo() {
  if (!auth()) return;
  String j = "{\"name\":\"" + String(cfg::OS_NAME) + "\",\"version\":\"" + cfg::OS_VERSION +
             "\",\"volumes\":[";
  for (int i = 0; i < storage::VOL_COUNT; i++) {
    storage::Vol v = (storage::Vol)i;
    if (i) j += ",";
    j += "{\"key\":\"" + String(storage::volKey(v)) + "\",\"name\":\"" + storage::volName(v) +
         "\",\"ok\":" + (storage::ok(v) ? "true" : "false") +
         ",\"used\":" + String((double)storage::usedBytes(v), 0) +
         ",\"total\":" + String((double)storage::totalBytes(v), 0) + "}";
  }
  j += "],\"glide\":" + String(glide::present() ? "true" : "false") + "}";
  server_->send(200, "application/json", j);
}

void LanApp::handleList() {
  if (!auth()) return;
  storage::Vol vol;
  String path;
  if (!argPath("path", vol, path)) return;
  if (!storage::isDir(vol, path)) path = "/";
  File dir = storage::fs(vol).open(path);
  String j = "{\"path\":\"" + jsonEsc(path) + "\",\"entries\":[";
  bool first = true;
  int count = 0;
  while (dir && count < 500) {
    File f = dir.openNextFile();
    if (!f) break;
    String n = storage::baseName(String(f.name()));
    bool d = f.isDirectory();
    uint32_t sz = d ? 0 : (uint32_t)f.size();
    f.close();
    if (n.length() == 0 || n[0] == '.') continue;  // ocultos (ajustes, senhas Wi-Fi)
    if (!first) j += ",";
    first = false;
    j += "{\"n\":\"" + jsonEsc(n) + "\",\"d\":" + (d ? "1" : "0") + ",\"s\":" + String(sz) + "}";
    count++;
  }
  if (dir) dir.close();
  j += "]}";
  server_->send(200, "application/json", j);
}

void LanApp::handleDownload() {
  if (!auth()) return;
  storage::Vol vol;
  String path;
  if (!argPath("path", vol, path)) return;
  File f = storage::fs(vol).open(path, FILE_READ);
  if (!f || f.isDirectory()) {
    server_->send(404, "text/plain", "Arquivo nao encontrado");
    return;
  }
  String name = storage::baseName(path);
  name.replace("\"", "");
  server_->sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  server_->streamFile(f, mimeFor(name));
  f.close();
  lastAction_ = "Enviado: " + name;
  invalidate();
}

void LanApp::handleDelete() {
  if (!auth()) return;
  storage::Vol vol;
  String path;
  if (!argPath("path", vol, path)) return;
  if (path == "/") { server_->send(400, "text/plain", "Nao apaga a raiz"); return; }
  bool ok = storage::removeAny(vol, path);
  lastAction_ = String(ok ? "Apagado: " : "Falhou ao apagar: ") + storage::baseName(path);
  invalidate();
  server_->send(ok ? 200 : 500, "text/plain", ok ? "ok" : "Nao apagou tudo");
}

void LanApp::handleMkdir() {
  if (!auth()) return;
  storage::Vol vol;
  String path;
  if (!argPath("path", vol, path)) return;
  if (!storage::validName(storage::baseName(path))) {
    server_->send(400, "text/plain", "Nome invalido");
    return;
  }
  bool ok = storage::makeDir(vol, path);
  server_->send(ok ? 200 : 500, "text/plain", ok ? "ok" : "Nao criou a pasta");
}

void LanApp::handleRename() {
  if (!auth()) return;
  storage::Vol vol;
  String from, to;
  if (!argPath("from", vol, from)) return;
  to = server_->arg("to");
  if (!storage::safePath(to) || to.indexOf("/.") >= 0 ||
      !storage::validName(storage::baseName(to))) {
    server_->send(400, "text/plain", "Nome invalido");
    return;
  }
  bool ok = storage::renameAny(vol, from, to);
  server_->send(ok ? 200 : 500, "text/plain", ok ? "ok" : "Nao renomeou (ja existe?)");
}

void LanApp::handleUploadData() {
  HTTPUpload& up = server_->upload();
  Kernel& k = Kernel::get();

  if (up.status == UPLOAD_FILE_START) {
    upError_ = "";
    upBytes_ = 0;
    upDenied_ = !server_->authenticate(cfg::WEB_USER, pin_.c_str());
    if (upDenied_) return;
    storage::Vol vol;
    String dir = server_->arg("dir");
    String name = storage::baseName(up.filename);
    if (!storage::volFromKey(server_->arg("vol"), vol) || !storage::ok(vol)) {
      upError_ = "Volume indisponivel";
    } else if (!storage::safePath(dir) || dir.indexOf("/.") >= 0 || !storage::validName(name) ||
               name[0] == '.') {
      upError_ = "Nome ou pasta invalidos";
    } else {
      upFile_ = storage::fs(vol).open(storage::joinPath(dir, name), FILE_WRITE);
      if (!upFile_) upError_ = "Nao criou o arquivo";
    }
    lastAction_ = "Recebendo " + name;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (upDenied_ || !upFile_) return;
    if (upFile_.write(up.buf, up.currentSize) != up.currentSize) {
      upError_ = "Sem espaco?";
      upFile_.close();
      return;
    }
    upBytes_ += up.currentSize;
    uint32_t now = millis();
    if (now - upLastDraw_ > 200) {
      upLastDraw_ = now;
      k.drawProgress("Recebendo pela rede", up.filename + "  " + storage::humanSize(upBytes_),
                     -1);
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (upFile_) upFile_.close();
    if (!upDenied_ && upError_.length() == 0)
      lastAction_ = "Recebido: " + up.filename + " (" + storage::humanSize(upBytes_) + ")";
    k.requestRedraw();
    invalidate();
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (upFile_) upFile_.close();
    upError_ = "Envio interrompido";
    k.requestRedraw();
  }
}

void LanApp::handleUploadDone() {
  if (!auth()) return;
  if (upError_.length()) server_->send(500, "text/plain", upError_);
  else server_->send(200, "text/plain", "ok");
}

// ------------------------------------------------------------ tela

void LanApp::render(M5Canvas& c, const Rect& area) {
  const Theme& t = theme::get();
  const int x = 8;
  int y = area.y + 4;

  if (!serverOn_) {
    c.setFont(ui::body());
    c.setTextColor(t.fg);
    String st = String("Wi-Fi: ") + net::stateText();
    c.drawString(st.c_str(), x, y + 6);
    if (net::state() == net::State::Connecting || net::state() == net::State::Scanning) {
      int dots = (millis() / 300) % 4;
      c.fillRect(x, y + 30, 4 + dots * 12, 3, t.accent);
    }
    if (net::state() == net::State::Failed) {
      c.setFont(ui::small());
      c.setTextColor(t.warn);
      c.drawString(net::lastError().c_str(), x, y + 30);
    }
    c.setFont(ui::small());
    c.setTextColor(t.dim);
    c.drawString("w  escolher uma rede Wi-Fi", x, y + 50);
    c.drawString("a  criar rede propria (sem roteador)", x, y + 62);
    ui::drawHint(c, "w Wi-Fi   a rede propria   ` voltar");
    return;
  }

  const bool ap = net::state() == net::State::AccessPoint;
  const String url = "http://" + net::ip();

  // QR code: no modo rede propria, entra direto na rede; senao, abre a pagina
  String qr = ap ? "WIFI:T:WPA;S:" + net::ssid() + ";P:" + net::apPassword() + ";;" : url;
  const int qs = 84, qx = cfg::SCREEN_W - qs - 6, qy = area.y + 4;
  c.fillRect(qx - 2, qy - 2, qs + 4, qs + 4, theme::rgb(255, 255, 255));
  c.qrcode(qr.c_str(), qx, qy, qs);

  c.setFont(ui::small());
  c.setTextColor(t.dim);
  if (ap) {
    c.drawString("1) Entre na rede:", x, y);
    c.setTextColor(t.fg);
    c.drawString(net::ssid().c_str(), x + 6, y + 10);
    c.drawString(("senha " + net::apPassword()).c_str(), x + 6, y + 20);
    c.setTextColor(t.dim);
    c.drawString("2) Abra no navegador:", x, y + 32);
    y += 42;
  } else {
    c.drawString("Abra no navegador:", x, y);
    y += 10;
  }
  c.setFont(ui::body());
  c.setTextColor(t.accent);
  c.drawString(net::ip().c_str(), x, y);
  c.setFont(ui::small());
  c.setTextColor(t.dim);
  c.drawString((String("ou ") + cfg::HOSTNAME + ".local").c_str(), x, y + 17);

  int py = area.y + area.h - 34;
  c.setTextColor(t.dim);
  c.drawString((String("usuario ") + cfg::WEB_USER + "   PIN").c_str(), x, py);
  c.setFont(ui::title());
  c.setTextColor(t.fg);
  c.drawString(pin_.c_str(), x + 108, py - 4);

  c.setFont(ui::small());
  c.setTextColor(t.ok);
  c.drawString(ui::ellipsize(c, lastAction_, area.w - 16).c_str(), x, area.y + area.h - 22);
  ui::drawHint(c, "p novo PIN   w Wi-Fi   ` sair (desliga)");
}
