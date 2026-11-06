#include "SlotManager.h"

#include <Preferences.h>
#include <SD.h>
#include <esp_ota_ops.h>

#include "Storage.h"

static const char* NS = "orbslots";

static constexpr uint8_t  IMAGE_MAGIC = 0xE9;      // inicio de imagem ESP
static constexpr uint32_t MERGED_APP_OFFSET = 0x10000;
static constexpr uint32_t MERGED_PT_OFFSET  = 0x8000;
static constexpr size_t   CHUNK = 4096;

void SlotManager::begin() {
  slots_.clear();
  // ota_0, ota_1, ... na ordem da tabela
  for (int k = 0; k < 16; k++) {
    auto sub = (esp_partition_subtype_t)(ESP_PARTITION_SUBTYPE_APP_OTA_MIN + k);
    const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, sub, nullptr);
    if (!p) break;
    SlotInfo s;
    s.index = (int)slots_.size();
    s.part = p;
    slots_.push_back(s);
  }
  refreshAll();
}

void SlotManager::refreshAll() {
  for (int i = 0; i < count(); i++) refresh(i);
}

void SlotManager::refresh(int i) {
  SlotInfo& s = slots_[i];
  esp_app_desc_t desc;
  s.valid = (esp_ota_get_partition_description(s.part, &desc) == ESP_OK);
  s.version = s.valid ? String(desc.version) : String();
  s.buildDate = s.valid ? String(desc.date) : String();
  loadMeta(s);
  if (s.valid && s.name.length() == 0) {
    // Firmwares Arduino costumam vir com project_name generico; usa se nao houver nome
    s.name = String(desc.project_name);
  }
  if (!s.valid) {
    s.name = "";
    s.source = "";
  }
}

int SlotManager::firstEmpty() const {
  for (const auto& s : slots_)
    if (!s.valid) return s.index;
  return -1;
}

void SlotManager::loadMeta(SlotInfo& s) {
  Preferences p;
  if (!p.begin(NS, true)) return;
  char key[8];
  snprintf(key, sizeof(key), "n%d", s.index);
  s.name = p.getString(key, "");
  snprintf(key, sizeof(key), "f%d", s.index);
  s.source = p.getString(key, "");
  p.end();
}

void SlotManager::saveMeta(const SlotInfo& s) {
  Preferences p;
  if (!p.begin(NS, false)) return;
  char key[8];
  snprintf(key, sizeof(key), "n%d", s.index);
  p.putString(key, s.name);
  snprintf(key, sizeof(key), "f%d", s.index);
  p.putString(key, s.source);
  p.end();
}

void SlotManager::rename(int i, const String& name) {
  slots_[i].name = name;
  saveMeta(slots_[i]);
}

String SlotManager::prettyName(const String& fileName) {
  String n = storage::baseName(fileName);
  int dot = n.lastIndexOf('.');
  if (dot > 0) n = n.substring(0, dot);
  // "hazke-v1.4.0-app" -> "hazke"; "GLIDE" -> "GLIDE"
  for (int i = 1; i < (int)n.length(); i++) {
    if (n[i] == '-' || n[i] == '_' || n[i] == ' ') {
      n = n.substring(0, i);
      break;
    }
  }
  if (n.length() && n[0] >= 'a' && n[0] <= 'z') n.setCharAt(0, n[0] - 32);
  return n;
}

bool SlotManager::installFromFile(int i, fs::FS& fs, const String& path, String& err,
                                  Progress cb) {
  if (i < 0 || i >= count()) { err = "Slot invalido"; return false; }
  SlotInfo& s = slots_[i];

  File f = fs.open(path, FILE_READ);
  if (!f) { err = "Nao abriu o arquivo"; return false; }
  const size_t fileSize = f.size();

  // Detecta imagem completa (bootloader + particoes + app)
  uint32_t offset = 0;
  uint8_t b[2] = {0, 0};
  if (fileSize > MERGED_APP_OFFSET + 32) {
    f.seek(MERGED_PT_OFFSET);
    f.read(b, 2);
    uint8_t m = 0;
    f.seek(MERGED_APP_OFFSET);
    f.read(&m, 1);
    if (b[0] == 0xAA && b[1] == 0x50 && m == IMAGE_MAGIC) offset = MERGED_APP_OFFSET;
  }
  f.seek(offset);
  uint8_t first = 0;
  f.read(&first, 1);
  if (first != IMAGE_MAGIC) {
    f.close();
    err = "Nao e um firmware ESP32";
    return false;
  }

  const size_t imageSize = fileSize - offset;
  if (imageSize > s.part->size) {
    f.close();
    err = "App grande demais (" + storage::humanSize(imageSize) + " > " +
          storage::humanSize(s.part->size) + ")";
    return false;
  }

  uint8_t* buf = (uint8_t*)malloc(CHUNK);
  if (!buf) { f.close(); err = "Sem memoria"; return false; }

  // Se o otadata aponta para este slot, volta para o sistema antes de apagar
  if (bootSelected() == i) selectFactory();

  esp_ota_handle_t h = 0;
  esp_err_t e = esp_ota_begin(s.part, imageSize, &h);
  if (e != ESP_OK) {
    free(buf);
    f.close();
    err = String("esp_ota_begin: ") + esp_err_to_name(e);
    return false;
  }

  f.seek(offset);
  size_t done = 0;
  while (done < imageSize) {
    size_t want = imageSize - done;
    if (want > CHUNK) want = CHUNK;
    size_t got = f.read(buf, want);
    if (got == 0) { e = ESP_FAIL; err = "Erro lendo o SD"; break; }
    e = esp_ota_write(h, buf, got);
    if (e != ESP_OK) { err = String("esp_ota_write: ") + esp_err_to_name(e); break; }
    done += got;
    if (cb) cb(done, imageSize);
  }
  free(buf);
  f.close();

  if (e != ESP_OK) {
    esp_ota_abort(h);
    refresh(i);
    return false;
  }
  e = esp_ota_end(h);  // valida a imagem (cabecalho, chip, hash)
  if (e != ESP_OK) {
    err = String("Imagem invalida: ") + esp_err_to_name(e);
    refresh(i);
    return false;
  }

  s.source = storage::baseName(path);
  s.name = prettyName(path);
  saveMeta(s);
  refresh(i);
  return true;
}

bool SlotManager::launch(int i, String& err) {
  if (i < 0 || i >= count() || !slots_[i].valid) { err = "Slot vazio"; return false; }
  esp_err_t e = esp_ota_set_boot_partition(slots_[i].part);
  if (e != ESP_OK) { err = String("Nao deu para abrir: ") + esp_err_to_name(e); return false; }
  delay(50);
  esp_restart();
  return true;  // nao chega aqui
}

bool SlotManager::erase(int i, String& err) {
  if (i < 0 || i >= count()) { err = "Slot invalido"; return false; }
  SlotInfo& s = slots_[i];
  if (bootSelected() == i) selectFactory();
  // Apagar so o inicio ja invalida a imagem (e e bem mais rapido que 2,6 MB)
  esp_err_t e = esp_partition_erase_range(s.part, 0, 0x10000);
  if (e != ESP_OK) { err = esp_err_to_name(e); return false; }
  s.name = "";
  s.source = "";
  saveMeta(s);
  refresh(i);
  return true;
}

void SlotManager::selectFactory() {
  const esp_partition_t* f =
      esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, nullptr);
  if (f) esp_ota_set_boot_partition(f);
}

int SlotManager::bootSelected() const {
  const esp_partition_t* b = esp_ota_get_boot_partition();
  if (!b) return -1;
  for (const auto& s : slots_)
    if (s.part->address == b->address) return s.index;
  return -1;
}
