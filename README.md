# Órbita OS

Firmware completo para o **M5Stack Cardputer ADV**: um sistema só, sem instalar nada, com notas, gerenciador de arquivos, acesso pela rede e pelo USB, **controle de apresentação com cronômetro** e o sintetizador **[GLIDE](https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV)** embutido.

Detalhes do desenho em [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md).

## Baixar e gravar

**Binário pronto:** [Releases → latest → `orbita-full.bin`](../../releases/latest)

1. No Chrome, abra o [ESP Web Flasher](https://espressif.github.io/esptool-js/) e ligue o Cardputer ADV pelo USB‑C.
2. **Connect**. Se a placa não aparecer: desligue, segure **G0** e ligue o cabo.
3. Grave o `orbita-full.bin` no endereço **0x0**. Desligue e ligue.

> Gravar o `orbita-full.bin` apaga o que estava na flash (inclusive o M5Launcher). As notas e os ajustes do Órbita ficam numa área que a gravação não toca, e o cartão SD não é mexido.

## O que tem

| App | Atalho | O que faz |
|---|---|---|
| **GLIDE** | `g` | O sintetizador GLIDE original. Abre em ~1 s; o **BtnRst** volta ao Órbita |
| **Apresentação** | `a` | Passa slides por **Bluetooth** ou **USB** (o Cardputer vira teclado) + cronômetro ou tempo regressivo com alertas |
| **Notas** | `n` | Bloco de notas com editor de texto (no SD; sem cartão, na flash interna) |
| **Arquivos** | `f` | Gerenciador do SD e da flash interna: editar texto, copiar/recortar/colar, renomear, pastas |
| **Rede** | `r` | Arquivos pelo navegador do celular ou PC, no Wi‑Fi de casa ou numa rede própria do Cardputer, com PIN |
| **USB** | `u` | O cartão SD vira pendrive no computador |
| **Wi‑Fi**, **Ajustes**, **Sistema** | `w` `j` `s` | Redes salvas; brilho, cores, sons, abrir o GLIDE ao ligar; memória, bateria, partições |

## Controle de apresentação

1. Abra **Apresentação**. No computador, procure dispositivos Bluetooth e pareie com **Orbita Apresentador** (só na primeira vez). Ou ligue o cabo USB‑C.
2. Abra os slides (PowerPoint, Google Slides, Keynote, PDF…).

| Tecla | Ação |
|---|---|
| `/` `.` `Enter` `espaço` | próximo slide |
| `,` `;` `Del` | slide anterior |
| `i` | iniciar apresentação (F5) e o tempo |
| `h` | iniciar deste slide (Shift+F5) |
| `e` | encerrar (Esc) |
| `b` / `w` | tela preta / branca |
| `t` | começar/pausar o tempo · `r` zerar · `m` cronômetro ↔ regressivo |
| `-` `=` | minutos do regressivo (com **shift**: de 5 em 5) |
| `k` | enviar setas ou PageUp/PageDown |

O tempo começa sozinho no primeiro slide. No regressivo, o número fica amarelo nos últimos 5 minutos, vermelho no último, e pisca quando passa do tempo, com bipes em cada etapa. A tela não escurece enquanto o app está aberto.

## Teclas gerais

`,` `/` navegar · `;` `.` cima/baixo · `Enter` abrir · `` ` `` voltar · `Tab` opções · letra = atalho do app · **BtnRst** volta ao Órbita

No editor de texto as letras digitam normalmente; **fn** + `; , . /` move o cursor e **ctrl+s** salva.

## Compilar

A cada push, o **GitHub Actions** compila tudo e publica em **Releases → latest**.

No computador:
```bash
pip install platformio esptool
pio run -e cardputer-adv                                          # sistema
cd bootloader && idf.py set-target esp32s3 && idf.py bootloader   # bootloader (ESP-IDF 5.1+)
cd .. && tools/flash.sh /dev/ttyACM0                              # grava tudo
```

> Modo USB: o build padrão usa o USB em modo OTG (pendrive + teclado). Se o computador não gravar sozinho, desligue, segure **G0** e ligue o cabo. Sem esses recursos USB: `pio run -e cardputer-adv-sem-usb` (a apresentação continua por Bluetooth).

## Licenças

- **Órbita:** código deste repositório (sugestão: MIT).
- **GLIDE** (`third_party/glide/`): binário oficial sem modificação, de Charles Tobin (CHARL3X), sob a **PolyForm Noncommercial License 1.0.0** ([`LICENSE`](third_party/glide/LICENSE), https://polyformproject.org/licenses/noncommercial/1.0.0). Uso e compartilhamento livres para fins não comerciais; vender aparelhos ou produtos com o GLIDE exige a licença comercial do autor. As licenças das bibliotecas do GLIDE estão em [`NOTICES.txt`](third_party/glide/NOTICES.txt).

  Required Notice: Copyright (C) 2026 Charles Tobin (CHARL3X)
  (https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV)
