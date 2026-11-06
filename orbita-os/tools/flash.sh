#!/usr/bin/env bash
# Grava o Orbita completo no Cardputer ADV:
#   0x0      bootloader Orbita   (bootloader/build/bootloader/bootloader.bin)
#   0x8000   tabela de particoes (.pio/build/cardputer-adv/partitions.bin)
#   0x10000  sistema Orbita      (.pio/build/cardputer-adv/firmware.bin)
#
# Uso:
#   tools/flash.sh /dev/ttyACM0            grava tudo
#   tools/flash.sh /dev/ttyACM0 --os-only  grava so o sistema (mantem apps e saves)
#   tools/flash.sh --merge                 gera out/orbita-full.bin (gravar em 0x0
#                                          com o web flasher ou M5Burner)
#
# Precisa do esptool:  pip install esptool
# Se o Cardputer nao for detectado: desligue, segure o botao G0, ligue o cabo USB.
set -euo pipefail
cd "$(dirname "$0")/.."

BOOT=bootloader/build/bootloader/bootloader.bin
PART=.pio/build/cardputer-adv/partitions.bin
APP=.pio/build/cardputer-adv/firmware.bin
ESPTOOL=${ESPTOOL:-esptool.py}
COMMON=(--chip esp32s3)
FLASH=(--flash_mode dio --flash_size 8MB)

need() { [ -f "$1" ] || { echo "Arquivo faltando: $1 ($2)"; exit 1; }; }

if [ "${1:-}" = "--merge" ]; then
  need "$BOOT" "compile com: cd bootloader && idf.py bootloader"
  need "$PART" "compile com: pio run"
  need "$APP"  "compile com: pio run"
  mkdir -p out
  $ESPTOOL "${COMMON[@]}" merge_bin -o out/orbita-full.bin "${FLASH[@]}" \
    0x0 "$BOOT" 0x8000 "$PART" 0x10000 "$APP"
  cp "$APP" out/orbita-os.bin
  echo "Pronto: out/orbita-full.bin (gravar em 0x0) e out/orbita-os.bin (so o sistema, 0x10000)"
  exit 0
fi

PORT=${1:?"informe a porta, ex.: tools/flash.sh /dev/ttyACM0"}

if [ "${2:-}" = "--os-only" ]; then
  need "$APP" "compile com: pio run"
  $ESPTOOL "${COMMON[@]}" --port "$PORT" --baud 921600 write_flash "${FLASH[@]}" 0x10000 "$APP"
  exit 0
fi

need "$BOOT" "compile com: cd bootloader && idf.py bootloader"
need "$PART" "compile com: pio run"
need "$APP"  "compile com: pio run"

# Apaga o otadata para comecar sempre pelo sistema
$ESPTOOL "${COMMON[@]}" --port "$PORT" --baud 921600 write_flash "${FLASH[@]}" \
  0x0 "$BOOT" 0x8000 "$PART" 0x10000 "$APP"
$ESPTOOL "${COMMON[@]}" --port "$PORT" erase_region 0xE000 0x2000
echo "Orbita gravado. Desligue e ligue o Cardputer."
