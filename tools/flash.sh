#!/usr/bin/env bash
# Gera os binarios do Orbita (e opcionalmente grava direto pelo USB).
#
#   tools/flash.sh --merge             gera, em out/:
#       orbita-launcher.bin  bootloader + particoes + Orbita. E o arquivo para
#                            o M5Launcher (copie para o SD e instale), e tambem
#                            pode ser gravado sozinho em 0x0.
#       orbita-app.bin       so o programa (0x10000), para quem preferir.
#   tools/flash.sh /dev/ttyACM0        grava o Orbita sozinho, SEM o Launcher
#                                      (apaga o Launcher da flash)
#
# Precisa do esptool:  pip install esptool
set -euo pipefail
cd "$(dirname "$0")/.."

ENV=${ORBITA_ENV:-cardputer-adv}
B=.pio/build/$ENV
BOOT=$B/bootloader.bin
PART=$B/partitions.bin
APP=$B/firmware.bin
ESPTOOL=${ESPTOOL:-esptool}
command -v "$ESPTOOL" >/dev/null 2>&1 || ESPTOOL=esptool.py
COMMON=(--chip esp32s3)
FLASH=(--flash-mode dio --flash-size 8MB)

for f in "$BOOT" "$PART" "$APP"; do
  [ -f "$f" ] || { echo "Arquivo faltando: $f (compile com: pio run -e $ENV)"; exit 1; }
done

if [ "${1:-}" = "--merge" ]; then
  mkdir -p out
  $ESPTOOL "${COMMON[@]}" merge-bin -o out/orbita-launcher.bin "${FLASH[@]}" \
    0x0 "$BOOT" 0x8000 "$PART" 0x10000 "$APP"
  cp "$APP" out/orbita-app.bin
  echo "Pronto: out/orbita-launcher.bin e out/orbita-app.bin"
  exit 0
fi

PORT=${1:?"informe a porta (ex.: /dev/ttyACM0) ou use --merge"}
echo "Isto grava o Orbita SOZINHO e apaga o M5Launcher da flash. Ctrl+C para cancelar."
sleep 3
$ESPTOOL "${COMMON[@]}" --port "$PORT" --baud 921600 write-flash "${FLASH[@]}" \
  0x0 "$BOOT" 0x8000 "$PART" 0x10000 "$APP"
$ESPTOOL "${COMMON[@]}" --port "$PORT" erase-region 0xE000 0x2000
echo "Pronto. Desligue e ligue o Cardputer."
