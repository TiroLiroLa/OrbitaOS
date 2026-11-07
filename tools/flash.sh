#!/usr/bin/env bash
# Grava o Orbita completo no Cardputer ADV:
#   0x000000  bootloader Orbita   (bootloader/build/bootloader/bootloader.bin)
#   0x008000  tabela de particoes (.pio/build/cardputer-adv/partitions.bin)
#   0x020000  sistema Orbita      (.pio/build/cardputer-adv/firmware.bin)
#   0x320000  GLIDE original      (third_party/glide/GLIDE.bin)
#
# Uso:
#   tools/flash.sh /dev/ttyACM0            grava tudo
#   tools/flash.sh /dev/ttyACM0 --os-only  grava so o sistema (mantem notas e ajustes)
#   tools/flash.sh --merge                 gera out/orbita-full.bin (gravar em 0x0
#                                          com o web flasher ou M5Burner)
#
# A particao orbitafs (0x4A0000, notas e ajustes) nunca e apagada por este script.
# Precisa do esptool:  pip install esptool
# Se o Cardputer nao for detectado: desligue, segure o botao G0, ligue o cabo USB.
set -euo pipefail
cd "$(dirname "$0")/.."

ENV=${ORBITA_ENV:-cardputer-adv}
BOOT=bootloader/build/bootloader/bootloader.bin
PART=.pio/build/$ENV/partitions.bin
APP=.pio/build/$ENV/firmware.bin
GLIDE=third_party/glide/GLIDE.bin
ESPTOOL=${ESPTOOL:-esptool.py}
COMMON=(--chip esp32s3)
FLASH=(--flash_mode dio --flash_size 8MB)
ADDR_APP=0x20000
ADDR_GLIDE=0x320000
GLIDE_MAX=$((0x180000))

need() { [ -f "$1" ] || { echo "Arquivo faltando: $1 ($2)"; exit 1; }; }
check_glide() {
  need "$GLIDE" "veja third_party/glide/README.md"
  local s
  s=$(stat -c %s "$GLIDE" 2>/dev/null || stat -f %z "$GLIDE")
  [ "$s" -le "$GLIDE_MAX" ] || { echo "GLIDE.bin ($s bytes) nao cabe na particao glide"; exit 1; }
}

if [ "${1:-}" = "--merge" ]; then
  need "$BOOT" "compile com: cd bootloader && idf.py bootloader"
  need "$PART" "compile com: pio run"
  need "$APP"  "compile com: pio run"
  check_glide
  mkdir -p out
  # O espaco entre as partes sai como 0xFF (apagado): o otadata fica vazio e
  # o primeiro boot cai no Orbita.
  $ESPTOOL "${COMMON[@]}" merge_bin -o out/orbita-full.bin "${FLASH[@]}" \
    0x0 "$BOOT" 0x8000 "$PART" $ADDR_APP "$APP" $ADDR_GLIDE "$GLIDE"
  cp "$APP" out/orbita-os.bin
  cp third_party/glide/LICENSE out/GLIDE-LICENSE.txt
  cp third_party/glide/NOTICES.txt out/GLIDE-NOTICES.txt
  echo "Pronto: out/orbita-full.bin (gravar em 0x0) e out/orbita-os.bin (so o sistema, $ADDR_APP)"
  exit 0
fi

PORT=${1:?"informe a porta, ex.: tools/flash.sh /dev/ttyACM0"}

if [ "${2:-}" = "--os-only" ]; then
  need "$APP" "compile com: pio run"
  $ESPTOOL "${COMMON[@]}" --port "$PORT" --baud 921600 write_flash "${FLASH[@]}" $ADDR_APP "$APP"
  exit 0
fi

need "$BOOT" "compile com: cd bootloader && idf.py bootloader"
need "$PART" "compile com: pio run"
need "$APP"  "compile com: pio run"
check_glide

$ESPTOOL "${COMMON[@]}" --port "$PORT" --baud 921600 write_flash "${FLASH[@]}" \
  0x0 "$BOOT" 0x8000 "$PART" $ADDR_APP "$APP" $ADDR_GLIDE "$GLIDE"
# Apaga o otadata para comecar sempre pelo sistema
$ESPTOOL "${COMMON[@]}" --port "$PORT" erase_region 0x19000 0x2000
echo "Orbita gravado. Desligue e ligue o Cardputer."
