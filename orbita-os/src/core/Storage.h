#pragma once
#include <Arduino.h>
#include <FS.h>

#include <functional>

// Armazenamento do Orbita: dois "volumes".
//   Vol::SD    - cartao microSD (FAT32), o principal
//   Vol::Flash - LittleFS na particao "spiffs" (1 MB), funciona sem cartao.
//                E compartilhado com os apps dos slots (o Hazke salva aqui).
namespace storage {

enum class Vol : uint8_t { SD = 0, Flash = 1 };
constexpr int VOL_COUNT = 2;

bool begin();          // monta o SD (pode ser chamado de novo para tentar remontar)
bool beginFlash();     // monta o LittleFS interno
void unmountSD();
bool remountSD();
// Enquanto o cartao esta com o computador (modo pendrive), o sistema nao pode
// mexer nele: sdOk() passa a responder false.
void lockSD(bool locked);
bool sdLocked();

bool sdOk();
bool flashOk();
bool ok(Vol v);
fs::FS& fs(Vol v);
const char* volName(Vol v);
const char* volKey(Vol v);  // "sd" / "flash" (usado na API web)
bool volFromKey(const String& key, Vol& out);
uint64_t totalBytes(Vol v);
uint64_t usedBytes(Vol v);

// Pasta padrao das notas: SD se houver, senao a flash interna
Vol notesVol();
void ensureDirs();

// Caminhos
String joinPath(const String& dir, const String& name);
String parentPath(const String& path);
String baseName(const String& path);
bool   hasExt(const String& name, const char* ext);  // ext com ponto, ex.: ".bin"
bool   isTextName(const String& name);
bool   safePath(const String& path);   // absoluto e sem ".."
bool   validName(const String& name);  // nome de arquivo sem barras
String humanSize(uint64_t bytes);

// Operacoes
using Progress = std::function<void(const String& item, size_t done, size_t total)>;
bool isDir(Vol v, const String& path);
bool exists(Vol v, const String& path);
bool makeDir(Vol v, const String& path);
bool removeAny(Vol v, const String& path);  // arquivo ou pasta (recursivo)
bool copyAny(Vol sv, const String& src, Vol dv, const String& dst, String& err,
             Progress cb = nullptr);
bool moveAny(Vol sv, const String& src, Vol dv, const String& dst, String& err,
             Progress cb = nullptr);
bool renameAny(Vol v, const String& from, const String& to);

}  // namespace storage
