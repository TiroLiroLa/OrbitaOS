#pragma once
#include <Arduino.h>
#include <FS.h>
#include <esp_partition.h>

#include <functional>
#include <vector>

// Um "slot" e uma particao OTA (app0/app1) que guarda um firmware externo
// inteiro, sem modificacao: Hazke, GLIDE, Bruce, etc.
struct SlotInfo {
  int index = 0;
  const esp_partition_t* part = nullptr;
  bool valid = false;     // ha um firmware valido gravado
  String name;            // nome mostrado (editavel)
  String source;          // arquivo de onde foi instalado
  String version;         // da descricao da imagem (esp_app_desc_t)
  String buildDate;
};

class SlotManager {
 public:
  using Progress = std::function<void(size_t done, size_t total)>;

  void begin();
  int count() const { return (int)slots_.size(); }
  const SlotInfo& slot(int i) const { return slots_[i]; }
  void refresh(int i);
  void refreshAll();
  int firstEmpty() const;

  // Instala um .bin do SD no slot. Aceita imagem so do app (o normal para
  // launchers, ex.: GLIDE.bin, hazke-*-app.bin) ou imagem completa com
  // bootloader+particoes (ex.: hazke-*-cardputer.bin): nesse caso pula 0x10000.
  bool installFromFile(int i, fs::FS& fs, const String& path, String& err, Progress cb);

  bool launch(int i, String& err);   // seleciona o slot e reinicia (nao retorna se der certo)
  bool erase(int i, String& err);    // invalida o firmware do slot
  void rename(int i, const String& name);

  // Faz o proximo esp_restart() cair no sistema (particao factory)
  static void selectFactory();
  // Qual slot o otadata aponta agora (-1 = factory/nenhum)
  int bootSelected() const;

  static String prettyName(const String& fileName);

 private:
  void loadMeta(SlotInfo& s);
  void saveMeta(const SlotInfo& s);

  std::vector<SlotInfo> slots_;
};
