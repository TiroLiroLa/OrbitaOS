# Órbita OS

Firmware principal para o **M5Stack Cardputer ADV**: um núcleo com funções de conveniência (notas, arquivos, rede, USB) e um menu para **trocar de firmware** — feito para alternar entre o jogo **[Hazke](https://github.com/therezor/hazke)** e o sintetizador **[GLIDE](https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV)**, instalados lado a lado. O botão **RST** sempre traz de volta ao Órbita.

Detalhes do desenho em [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md).

## O que já faz

- **Apps**: dois slots de 2,5 MB para firmwares completos; abrir, substituir, renomear, apagar. O **BtnRst** volta ao Órbita
- **Notas**: bloco de notas com editor de texto (no SD ou, sem cartão, na flash interna)
- **Arquivos**: gerenciador do SD e da flash interna — editar texto, copiar/recortar/colar, renomear, pastas, instalar `.bin`
- **Rede**: gerenciador de arquivos pelo navegador do celular ou PC (enviar, baixar, apagar, instalar e abrir firmware), no Wi‑Fi de casa ou numa rede própria do Cardputer, protegido por PIN
- **USB**: o cartão SD vira pendrive no computador
- **Wi‑Fi**, **Ajustes** e **Sistema**

## Compilar

### Opção A — na nuvem (sem instalar nada)
1. Suba esta pasta para um repositório no GitHub.
2. A aba **Actions** compila sozinha (bootloader com ESP‑IDF + sistema com PlatformIO).
3. Baixe o artefato **orbita**: tem `orbita-full.bin` (tudo junto) e as partes separadas.

### Opção B — no computador
```bash
# sistema
pip install platformio esptool
pio run -e cardputer-adv

# bootloader (precisa do ESP-IDF 5.1+)
cd bootloader && idf.py set-target esp32s3 && idf.py bootloader && cd ..
```

## Gravar no Cardputer

Ligue o Cardputer ADV ao computador pelo USB‑C (se não aparecer porta: desligue, segure **G0**, ligue o cabo).

```bash
tools/flash.sh /dev/ttyACM0            # tudo: bootloader + partições + Órbita
tools/flash.sh /dev/ttyACM0 --os-only  # só o sistema (mantém apps e saves)
```

Ou grave o `orbita-full.bin` no endereço **0x0** com o [ESP Web Flasher](https://espressif.github.io/esptool-js/) pelo Chrome.

> ⚠️ Gravar o Órbita apaga o que estava na flash (inclusive o M5Launcher e saves internos de apps). Saves no cartão SD não são tocados.
>
> `pio run -t upload` funciona, mas grava o bootloader **padrão** por cima — aí o RST deixa de voltar ao sistema (a barra de status mostra **BL!**). Use o `flash.sh`.

## Instalar o Hazke e o GLIDE

1. Baixe das releases oficiais:
   - Hazke: `hazke-v1.4.0-app.bin` (o `-cardputer.bin` completo também funciona)
   - GLIDE: `GLIDE.bin`
2. Copie para **`/orbita/apps/`** no cartão SD (FAT32).
   O jeito mais fácil: app **USB** (o cartão vira pendrive) ou app **Rede** (arraste pelo navegador).
3. No Órbita: **Apps** → **Instalar app do cartão SD** → escolha o arquivo → escolha o slot.
4. Abra pelo carrossel ou pelos atalhos `1` e `2`.

## Teclas

`,` `/` navegar · `;` `.` cima/baixo · `Enter` abrir · `` ` `` voltar · `Tab` opções · `1`–`9` abrir slot · **BtnRst** volta ao Órbita

No editor de texto as letras digitam normalmente; **fn** + `; , . /` move o cursor e **ctrl+s** salva. Tabela completa em [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md).

> Modo USB: o build padrão usa o USB em modo OTG para virar pendrive. Se o computador não gravar sozinho, desligue, segure **G0** e ligue o cabo. Sem pendrive: `pio run -e cardputer-adv-sem-usb`.

## Licenças

O código do Órbita é seu para licenciar (sugestão: MIT). Hazke é MIT. GLIDE é PolyForm Noncommercial 1.0 — uso pessoal livre; o Órbita não redistribui o GLIDE, apenas instala o binário oficial que você copia para o cartão.
