# Órbita OS — arquitetura e roteiro

O Órbita é um **app para o M5Launcher** no **M5Stack Cardputer ADV**. A divisão de papéis:

| Quem | Faz |
|---|---|
| **M5Launcher** | boot e troca de firmware (Órbita, GLIDE…), arquivos por USB e pela rede (WUI), Wi‑Fi, instalar e atualizar apps |
| **Órbita** | controle de apresentação com cronômetro, notas, gerenciador de arquivos no aparelho, ajustes |

Hardware: ESP32‑S3FN8 a 240 MHz, 8 MB de flash, **sem PSRAM** (~512 KB de SRAM), tela ST7789V2 240×135, teclado 56 teclas (TCA8418), áudio ES8311, IMU BMI270, IR, microSD, Bluetooth LE, USB OTG.

## 1. Instalação e partições

O release traz dois arquivos:

- **`orbita-launcher.bin`**: bootloader, tabela de partições e Órbita juntos. O Launcher atual lê a tabela de dentro do arquivo e cria as partições do app com os nomes dela, inclusive as de dados.
- **`orbita-app.bin`**: só o programa. Nesse caso o Launcher não cria a flash interna, e as notas ficam só no SD.

Tabela (`partitions.csv`):

```
nvs        data  nvs        20 KB   ajustes (namespace "orbita"), vínculo Bluetooth
otadata    data  ota         8 KB
app0       app   ota_0       2 MB   o Órbita (~1,2 MB hoje)
orbitafs   data  littlefs  5,9 MB   "flash interna": notas e arquivos sem cartão
coredump   data  coredump   64 KB
```

- A `orbitafs` usa o subtipo **littlefs (0x83)**, não spiffs. Assim, um firmware que procura uma partição SPIFFS, e às vezes a formata, não a encontra por engano.
- O Launcher pode redimensionar a `orbitafs` na instalação. O Órbita monta pelo nome e se adapta ao tamanho que encontrar.
- Se a partição não existir (por exemplo, com o `orbita-app.bin`), **Arquivos** mostra só o SD.
- Os ajustes ficam na **NVS**, compartilhada com o Launcher e os outros apps (padrão do Launcher). São poucos bytes e, se sumirem, voltam ao padrão.

**Voltar ao Launcher:** ligue o aparelho ou aperte **BtnRst** e aperte uma tecla na tela do Launcher. O item **Reiniciar o Órbita** (em Ajustes) usa reset por software, então volta direto para o Órbita.

## 2. Apps

| App | Atalho | O que faz |
|---|---|---|
| **Apresentação** | `a` | teclado Bluetooth LE + USB para passar slides; cronômetro / regressivo com alertas |
| **Notas** | `n` | notas `.txt` em `/orbita/notas` (SD; sem cartão, flash interna) |
| **Arquivos** | `f` | SD e flash interna: editar texto, copiar/recortar/colar entre volumes, renomear, pastas, apagar |
| **Ajustes** | `j` | brilho, escurecer, cor, sons, reiniciar |
| **Sistema** | `s` | RAM, bateria, armazenamento, Bluetooth, partições |

### Controle de apresentação

- **Bluetooth:** `BLE HID`, nome "Orbita Apresentador", pareamento "Just Works" com vínculo (o computador reconecta sozinho). Só liga com o app aberto e para de anunciar ao sair.
- **USB:** teclado TinyUSB no build padrão (`cardputer-adv`, USB em modo OTG). O vínculo do Bluetooth fica na NVS.
- **Teclas enviadas:**
  - setas ← → (ou PageUp/PageDown, tecla `k`)
  - F5 (iniciar) e Shift+F5 (deste slide)
  - Esc (encerrar)
  - B (tela preta) e W (tela branca)
- **Tempo:** começa no primeiro slide. No regressivo há bipes a 5 min do fim, a 1 min e no zero; depois disso o número pisca com "+".
- A tela não escurece com o app aberto (`App::keepAwake()`).

## 3. Código

```
src/
├── main.cpp
├── core/
│   ├── Kernel        pilha de apps, laço, energia, barra de status
│   ├── App.h         classe base (render, onKey, update, keepAwake...)
│   ├── Input, UI, Dialogs, Theme
│   ├── Storage       volumes SD + flash (orbitafs), copiar/mover/apagar recursivo
│   ├── Settings      ajustes na NVS
│   └── Hid           teclado Bluetooth LE + USB
└── apps/
    ├── HomeApp, PresenterApp, NotesApp, EditorApp, FilesApp, SettingsApp, SystemApp
tools/flash.sh        gera orbita-launcher.bin / orbita-app.bin (ou grava direto, sem Launcher)
```

## 4. Roteiro

- **Fonte com acentos** (Latin‑1) na interface
- **Apresentação:** ponteiro virtual (mouse HID movido pelo IMU), anotações do apresentador lidas de um `.txt` do SD, relógio
- Controle remoto IR, gravador de voz, nível (IMU), player de WAV
- Ícone do Órbita na lista de apps do Launcher

## 5. Checklist no aparelho

1. Instalar o `orbita-launcher.bin` pelo Launcher → o Órbita abre; **Sistema** mostra a flash interna com espaço.
2. BtnRst → tela do Launcher → escolher o GLIDE → BtnRst → escolher o Órbita.
3. **Apresentação:** parear "Orbita Apresentador"; `/` e `,` passam slides; `i` inicia; `b` tela preta; regressivo de 1 min apita e pisca.
4. **Apresentação pelo cabo USB**, sem Bluetooth.
5. **Notas** e **Arquivos** no SD e na flash interna.
