#include "Storage.h"

#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>

#include <vector>

#include "Config.h"

namespace storage {

static bool sdMounted = false;
static bool flashMounted = false;
static bool spiStarted = false;
static bool sdLock = false;

// ------------------------------------------------------------ montagem

bool begin() {
  if (sdLock) return false;
  if (sdMounted) return true;
  if (!spiStarted) {
    SPI.begin(cfg::SD_SCK, cfg::SD_MISO, cfg::SD_MOSI, cfg::SD_CS);
    spiStarted = true;
  }
  sdMounted = SD.begin(cfg::SD_CS, SPI, 25000000);
  if (sdMounted && SD.cardType() == CARD_NONE) {
    SD.end();
    sdMounted = false;
  }
  if (sdMounted) ensureDirs();
  return sdMounted;
}

bool beginFlash() {
  if (flashMounted) return true;
  // Mesmo comportamento do Hazke: formata so se a particao ainda nao tiver
  // um LittleFS valido (primeiro uso).
  flashMounted = LittleFS.begin(true, "/littlefs", 10, "spiffs");
  if (flashMounted) ensureDirs();
  return flashMounted;
}

void unmountSD() {
  if (sdMounted) SD.end();
  sdMounted = false;
}

bool remountSD() {
  unmountSD();
  return begin();
}

bool sdOk() { return sdMounted && !sdLock; }
void lockSD(bool locked) { sdLock = locked; }
bool sdLocked() { return sdLock; }
bool flashOk() { return flashMounted; }
bool ok(Vol v) { return v == Vol::SD ? sdOk() : flashMounted; }

fs::FS& fs(Vol v) {
  if (v == Vol::Flash) return LittleFS;
  return SD;
}

const char* volName(Vol v) { return v == Vol::SD ? "Cartao SD" : "Flash interna"; }
const char* volKey(Vol v) { return v == Vol::SD ? "sd" : "flash"; }

bool volFromKey(const String& key, Vol& out) {
  if (key == "sd") { out = Vol::SD; return true; }
  if (key == "flash") { out = Vol::Flash; return true; }
  return false;
}

uint64_t totalBytes(Vol v) {
  if (!ok(v)) return 0;
  return v == Vol::SD ? SD.totalBytes() : LittleFS.totalBytes();
}

uint64_t usedBytes(Vol v) {
  if (!ok(v)) return 0;
  return v == Vol::SD ? SD.usedBytes() : LittleFS.usedBytes();
}

Vol notesVol() { return sdOk() ? Vol::SD : Vol::Flash; }

void ensureDirs() {
  if (sdMounted) {
    if (!SD.exists(cfg::DIR_ROOT)) SD.mkdir(cfg::DIR_ROOT);
    if (!SD.exists(cfg::DIR_APPS)) SD.mkdir(cfg::DIR_APPS);
    if (!SD.exists(cfg::DIR_NOTES)) SD.mkdir(cfg::DIR_NOTES);
  }
  if (flashMounted) {
    if (!LittleFS.exists(cfg::DIR_ROOT)) LittleFS.mkdir(cfg::DIR_ROOT);
    if (!LittleFS.exists(cfg::DIR_NOTES)) LittleFS.mkdir(cfg::DIR_NOTES);
  }
}

// ------------------------------------------------------------ caminhos

String joinPath(const String& dir, const String& name) {
  if (dir.endsWith("/")) return dir + name;
  return dir + "/" + name;
}

String parentPath(const String& path) {
  if (path == "/" || path.length() == 0) return "/";
  int i = path.lastIndexOf('/');
  if (i <= 0) return "/";
  return path.substring(0, i);
}

String baseName(const String& path) {
  int i = path.lastIndexOf('/');
  return i < 0 ? path : path.substring(i + 1);
}

bool hasExt(const String& name, const char* ext) {
  String n = name;
  n.toLowerCase();
  return n.endsWith(ext);
}

bool isTextName(const String& name) {
  static const char* exts[] = {".txt", ".md", ".log", ".json", ".csv", ".ini", ".cfg",
                               ".conf", ".yml", ".yaml", ".xml", ".html", ".htm", ".js",
                               ".css", ".py", ".lua", ".c", ".h", ".cpp", ".sh", ".ir",
                               ".sub", ".gpat"};
  for (const char* e : exts)
    if (hasExt(name, e)) return true;
  return false;
}

bool safePath(const String& path) {
  if (path.length() == 0 || path[0] != '/') return false;
  if (path.indexOf("..") >= 0) return false;
  if (path.indexOf("//") >= 0) return false;
  return true;
}

bool validName(const String& name) {
  if (name.length() == 0 || name.length() > 64) return false;
  if (name == "." || name == "..") return false;
  for (unsigned i = 0; i < name.length(); i++) {
    char c = name[i];
    if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' ||
        c == '>' || c == '|' || (uint8_t)c < 32)
      return false;
  }
  return true;
}

String humanSize(uint64_t b) {
  char buf[24];
  if (b < 1024) snprintf(buf, sizeof(buf), "%u B", (unsigned)b);
  else if (b < 1024ULL * 1024) snprintf(buf, sizeof(buf), "%.1f KB", b / 1024.0);
  else if (b < 1024ULL * 1024 * 1024) snprintf(buf, sizeof(buf), "%.2f MB", b / 1048576.0);
  else snprintf(buf, sizeof(buf), "%.2f GB", b / 1073741824.0);
  return String(buf);
}

// ------------------------------------------------------------ operacoes

bool exists(Vol v, const String& path) { return ok(v) && fs(v).exists(path); }

bool isDir(Vol v, const String& path) {
  if (!ok(v)) return false;
  File f = fs(v).open(path);
  if (!f) return false;
  bool d = f.isDirectory();
  f.close();
  return d;
}

bool makeDir(Vol v, const String& path) { return ok(v) && fs(v).mkdir(path); }

static void listChildren(Vol v, const String& dir, std::vector<String>& out) {
  File d = fs(v).open(dir);
  if (!d || !d.isDirectory()) return;
  while (true) {
    File f = d.openNextFile();
    if (!f) break;
    out.push_back(joinPath(dir, baseName(String(f.name()))));
    f.close();
  }
  d.close();
}

bool removeAny(Vol v, const String& path) {
  if (!ok(v) || path == "/" || !safePath(path)) return false;
  if (!isDir(v, path)) return fs(v).remove(path);
  std::vector<String> kids;
  listChildren(v, path, kids);  // lista antes de apagar (nao apaga durante a iteracao)
  bool all = true;
  for (const String& k : kids) all &= removeAny(v, k);
  return fs(v).rmdir(path) && all;
}

static bool copyFile(Vol sv, const String& src, Vol dv, const String& dst, String& err,
                     Progress cb) {
  File in = fs(sv).open(src, FILE_READ);
  if (!in) { err = "Nao abriu " + baseName(src); return false; }
  File out = fs(dv).open(dst, FILE_WRITE);
  if (!out) { in.close(); err = "Nao criou " + baseName(dst); return false; }
  const size_t total = in.size();
  const size_t CH = 4096;
  uint8_t* buf = (uint8_t*)malloc(CH);
  if (!buf) { in.close(); out.close(); err = "Sem memoria"; return false; }
  size_t done = 0;
  bool okk = true;
  String item = baseName(src);
  while (done < total) {
    size_t n = in.read(buf, CH);
    if (n == 0) { okk = false; err = "Erro de leitura"; break; }
    if (out.write(buf, n) != n) { okk = false; err = "Sem espaco no destino?"; break; }
    done += n;
    if (cb) cb(item, done, total);
  }
  free(buf);
  in.close();
  out.close();
  if (!okk) fs(dv).remove(dst);
  return okk;
}

bool copyAny(Vol sv, const String& src, Vol dv, const String& dst, String& err, Progress cb) {
  if (!ok(sv) || !ok(dv)) { err = "Volume indisponivel"; return false; }
  if (!safePath(src) || !safePath(dst)) { err = "Caminho invalido"; return false; }
  if (sv == dv && (dst == src || dst.startsWith(src + "/"))) {
    err = "Destino dentro da origem";
    return false;
  }
  if (exists(dv, dst)) { err = baseName(dst) + " ja existe"; return false; }
  if (!isDir(sv, src)) return copyFile(sv, src, dv, dst, err, cb);

  if (!fs(dv).mkdir(dst)) { err = "Nao criou a pasta"; return false; }
  std::vector<String> kids;
  listChildren(sv, src, kids);
  for (const String& k : kids) {
    if (!copyAny(sv, k, dv, joinPath(dst, baseName(k)), err, cb)) return false;
  }
  return true;
}

bool moveAny(Vol sv, const String& src, Vol dv, const String& dst, String& err, Progress cb) {
  if (sv == dv) {
    if (exists(dv, dst)) { err = baseName(dst) + " ja existe"; return false; }
    if (dst.startsWith(src + "/")) { err = "Destino dentro da origem"; return false; }
    if (fs(sv).rename(src, dst)) return true;
    err = "Nao deu para mover";
    return false;
  }
  if (!copyAny(sv, src, dv, dst, err, cb)) return false;
  removeAny(sv, src);
  return true;
}

bool renameAny(Vol v, const String& from, const String& to) {
  if (!ok(v) || exists(v, to)) return false;
  return fs(v).rename(from, to);
}

}  // namespace storage
