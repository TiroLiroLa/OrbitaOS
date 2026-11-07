#!/usr/bin/env bash
# Atualiza o GLIDE embutido com a versao mais recente do repositorio oficial.
# Uso: tools/update-glide.sh
set -euo pipefail
cd "$(dirname "$0")/.."
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
git clone -q --depth 1 https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV "$TMP/glide"
cp "$TMP/glide/dist/GLIDE.bin" "$TMP/glide/dist/NOTICES.txt" "$TMP/glide/LICENSE" third_party/glide/
SHA=$(git -C "$TMP/glide" rev-parse HEAD)
SUM=$(sha256sum third_party/glide/GLIDE.bin | cut -d' ' -f1)
sed -i -E "s/^\| Commit de origem \|.*/| Commit de origem | \`$SHA\` |/; s/^\| SHA-256 \|.*/| SHA-256 | \`$SUM\` |/" third_party/glide/README.md
SIZE=$(stat -c %s third_party/glide/GLIDE.bin)
echo "GLIDE atualizado: $SIZE bytes (commit $SHA)"
[ "$SIZE" -le 1572864 ] || echo "ATENCAO: maior que a particao glide (1,5 MB)! Aumente-a em partitions.csv."
