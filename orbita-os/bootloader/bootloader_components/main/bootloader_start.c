/*
 * Orbita OS - bootloader de 2o estagio customizado (ESP32-S3)
 *
 * Baseado no exemplo oficial do ESP-IDF:
 *   examples/custom_bootloader/bootloader_override
 *
 * Unica mudanca de comportamento em relacao ao bootloader padrao:
 *
 *   - Reset por software (esp_restart) ou despertar de deep sleep:
 *       respeita a escolha do otadata -> o app escolhido pelo Orbita roda.
 *   - Qualquer outro reset (ligar, botao RST, USB, watchdog de hardware...):
 *       ignora o otadata e carrega a particao "factory" -> volta ao Orbita.
 *
 * Assim o Orbita abre um app com esp_ota_set_boot_partition() + esp_restart(),
 * e o botao RST sempre traz o usuario de volta, sem precisar mexer no app.
 * Apps que reiniciam a si mesmos (ex.: GLIDE segurando a tecla `) continuam
 * reiniciando neles mesmos, porque isso tambem e um reset por software.
 */
#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "bootloader_common.h"

#ifndef FACTORY_INDEX
#define FACTORY_INDEX (-1)
#endif
#ifndef INVALID_INDEX
#define INVALID_INDEX (-99)
#endif

/* O sistema procura esta assinatura nos primeiros 32 KB da flash para saber
 * se o bootloader Orbita esta instalado. Nao mude sem mudar cfg::BOOT_MARK. */
static const char ORBITA_MARK[] = "ORBITA-BOOT-1";

static const char *TAG = "orbita";

static int select_partition_number(bootloader_state_t *bs);

void __attribute__((noreturn)) call_start_cpu0(void)
{
    // 1. Inicializacao do hardware
    if (bootloader_init() != ESP_OK) {
        bootloader_reset();
    }

#ifdef CONFIG_BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP
    // Despertar de deep sleep: tenta o caminho curto, igual ao padrao.
    bootloader_utility_load_boot_image_from_deep_sleep();
#endif

    // 2. Escolhe a particao
    bootloader_state_t bs = {0};
    int boot_index = select_partition_number(&bs);
    if (boot_index == INVALID_INDEX) {
        bootloader_reset();
    }

    // 3. Carrega a imagem escolhida
    bootloader_utility_load_boot_image(&bs, boot_index);
}

static bool reset_keeps_app(soc_reset_reason_t rr)
{
    switch (rr) {
    case RESET_REASON_CORE_SW:          // esp_restart()
    case RESET_REASON_CPU0_SW:          // reset de CPU por software
    case RESET_REASON_CORE_DEEP_SLEEP:  // despertar de deep sleep
        return true;
    default:
        return false;
    }
}

static int select_partition_number(bootloader_state_t *bs)
{
    if (!bootloader_utility_load_partition_table(bs)) {
        ESP_LOGE(TAG, "erro ao ler a tabela de particoes");
        return INVALID_INDEX;
    }

    soc_reset_reason_t rr = esp_rom_get_reset_reason(0);
    // Leitura volatil: garante que a assinatura nao seja descartada pelo linker
    // mesmo se o nivel de log for reduzido.
    volatile char keep = ORBITA_MARK[0];
    (void)keep;

    if (!reset_keeps_app(rr) && bs->factory.offset != 0) {
        ESP_LOGI(TAG, "%s: reset %d -> Orbita (factory)", ORBITA_MARK, (int)rr);
        return FACTORY_INDEX;
    }

    ESP_LOGI(TAG, "%s: reset %d -> particao do otadata", ORBITA_MARK, (int)rr);
    return bootloader_utility_get_selected_boot_partition(bs);
}

// Retorna a estrutura reent global caso funcoes da newlib sejam linkadas
// (igual ao bootloader original). IDFs novos podem usar picolibc, que nao tem isso.
#if !defined(CONFIG_LIBC_PICOLIBC)
struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;
}
#endif
