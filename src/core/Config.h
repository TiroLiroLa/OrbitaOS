#pragma once
#include <stdint.h>

// Configuracoes fixas do Orbita OS.
// Obs.: a fonte padrao do M5GFX so tem ASCII, por isso os textos da interface
// estao sem acento por enquanto (ver "Fontes" no roadmap em docs/ARQUITETURA.md).
namespace cfg {

constexpr const char* OS_NAME    = "Orbita";
constexpr const char* OS_VERSION = "0.3.0";

constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 135;
constexpr int STATUS_H = 14;   // barra de status no topo

// Pastas
constexpr const char* DIR_ROOT     = "/orbita";
constexpr const char* DIR_NOTES    = "/orbita/notas";   // no SD ou na flash interna
constexpr const char* DIR_SYS      = "/.orbita";        // flash interna, oculta
constexpr const char* FILE_SETTINGS = "/.orbita/ajustes";
constexpr const char* FILE_WIFI     = "/.orbita/wifi";

// Particoes (ver partitions.csv)
constexpr const char* GLIDE_PARTITION = "glide";
constexpr const char* FLASH_FS_LABEL  = "orbitafs";

// Rede
constexpr const char* HOSTNAME     = "orbita";         // http://orbita.local
constexpr const char* WEB_USER     = "orbita";         // usuario da pagina web (senha = PIN na tela)
constexpr uint32_t    WIFI_TIMEOUT_MS = 15000;
constexpr int         MAX_KNOWN_WIFI  = 5;

// Pinos do microSD (iguais no Cardputer e no Cardputer ADV)
constexpr int SD_SCK  = 40;
constexpr int SD_MISO = 39;
constexpr int SD_MOSI = 14;
constexpr int SD_CS   = 12;

// Assinatura gravada no bootloader Orbita (bootloader/.../bootloader_start.c)
constexpr const char* BOOT_MARK = "ORBITA-BOOT-1";
constexpr uint32_t    BOOT_SCAN_BYTES = 0x8000;  // bootloader ocupa 0x0..0x8000

// Repeticao de tecla segurada
constexpr uint32_t KEY_REPEAT_DELAY_MS = 420;
constexpr uint32_t KEY_REPEAT_RATE_MS  = 80;

constexpr uint32_t FRAME_MS = 33;  // ~30 fps para apps animados

}  // namespace cfg
