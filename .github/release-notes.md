## Como gravar

1. No Chrome, abra o [ESP Web Flasher](https://espressif.github.io/esptool-js/) e ligue o Cardputer ADV pelo USB-C.
2. **Connect** (se nao aparecer: desligue, segure **G0** e ligue o cabo).
3. Grave **`orbita-full.bin`** no endereco **`0x0`**. Desligue e ligue.

| Arquivo | Para que serve |
|---|---|
| `orbita-full.bin` | Tudo junto (bootloader, Orbita e GLIDE). Gravar em `0x0`. |
| `orbita-os.bin` | So o sistema, para atualizar sem apagar notas e ajustes. Gravar em `0x20000`. |
| `bootloader.bin`, `partitions.bin` | Partes separadas (`0x0` e `0x8000`). |

Gravar o `orbita-full.bin` apaga o que estava na flash, menos as notas e os ajustes do Orbita. O cartao SD nao e tocado.

## GLIDE embutido

O `orbita-full.bin` inclui o **GLIDE** original, sem modificacao, de Charles Tobin (CHARL3X):
https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV

O GLIDE e distribuido sob a **PolyForm Noncommercial License 1.0.0** (`GLIDE-LICENSE.txt`,
https://polyformproject.org/licenses/noncommercial/1.0.0): uso livre para fins nao comerciais.
As licencas das bibliotecas que ele usa estao em `GLIDE-NOTICES.txt`.

Required Notice: Copyright (C) 2026 Charles Tobin (CHARL3X)
(https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV)
