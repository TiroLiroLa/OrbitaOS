#include "Glide.h"

#include <esp_image_format.h>
#include <esp_ota_ops.h>

#include "Config.h"

namespace glide {

const esp_partition_t* partition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY,
                                  cfg::GLIDE_PARTITION);
}

bool present() {
  const esp_partition_t* p = partition();
  if (!p) return false;
  esp_app_desc_t desc;
  return esp_ota_get_partition_description(p, &desc) == ESP_OK;
}

uint32_t imageSize() {
  const esp_partition_t* p = partition();
  if (!p || !present()) return 0;
  esp_image_metadata_t meta;
  const esp_partition_pos_t pos = {p->address, p->size};
  if (esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &pos, &meta) != ESP_OK) return 0;
  return meta.image_len;
}

bool launch(String& err) {
  const esp_partition_t* p = partition();
  if (!p || !present()) {
    err = "GLIDE nao esta gravado (use o orbita-full.bin)";
    return false;
  }
  esp_err_t e = esp_ota_set_boot_partition(p);
  if (e != ESP_OK) {
    err = String("Nao deu para abrir: ") + esp_err_to_name(e);
    return false;
  }
  delay(50);
  esp_restart();
  return true;  // nao chega aqui
}

void selectSystem() {
  const esp_partition_t* f =
      esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, nullptr);
  if (f) esp_ota_set_boot_partition(f);
}

bool bootSelected() {
  const esp_partition_t* b = esp_ota_get_boot_partition();
  const esp_partition_t* p = partition();
  return b && p && b->address == p->address;
}

}  // namespace glide
