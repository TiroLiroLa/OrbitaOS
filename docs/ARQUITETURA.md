# Órbita OS — arquitetura e roteiro

O Órbita é um **firmware único e completo** para o **M5Stack Cardputer ADV**. Ele não instala nada: tudo o que ele faz já vem no arquivo de gravação, inclusive o sintetizador **GLIDE** original.

Hardware: ESP32‑S3FN8 a 240 MHz, 8 MB de flash, **sem PSRAM** (~512 KB de SRAM), tela ST7789V2 240×135, teclado 56 teclas (TCA8418), áudio ES8311, IMU BMI270, IR, microSD, Bluetooth LE, USB OTG.

## 1. O GLIDE dentro do Órbita

O repositório do GLIDE (CHARL3X) publica só o binário (`dist/GLIDE.bin`), não o código-fonte. Então ele não pode ser compilado como app nativo. Em vez disso:

- O `GLIDE.bin` oficial, **sem modificação**, fica em `third_party/glide/` e é gravado numa partição fixa (`glide`) junto com o Órbita, no mesmo `orbita-full.bin`.
- No menu, **GLIDE** aponta o `otadata` para essa partição e reinicia (~1 s).
- O **bootloader do Órbita** volta para o sistema em qualquer reset por botão ou ao ligar. Já um reinício por software, como o do GLIDE ao segurar `` ` ``, continua no GLIDE.

Licença: PolyForm Noncommercial 1.0.0. Redistribuir o binário para fins não comerciais é permitido, desde que vão junto o texto da licença, a linha `Required Notice` e o `NOTICES.txt`. O build já coloca esses três na página Releases. O nome GLIDE só aparece para se referir ao GLIDE original; o ícone do menu é desenho próprio.

### Cuidado com a NVS

O GLIDE guarda os ajustes dele na partição **NVS** e, se ela encher, limpa a área inteira sozinho. Por isso:
- O Órbita **não usa a NVS** para os ajustes nem para as senhas de Wi‑Fi. Esses dados ficam em `/.orbita/` na flash interna (LittleFS); o Wi‑Fi roda com `WiFi.persistent(false)`.
- A NVS ficou com 64 KB, para encher bem menos.
- O que continua na NVS, e pode sumir se o GLIDE limpar a área: o vínculo do Bluetooth (aí é só parear de novo) e a calibração de rádio (refeita sozinha).

A flash interna usa um **subtipo próprio (0x40)** de partição. Assim, nenhum firmware que procure uma partição "spiffs" consegue formatá-la por engano.

## 2. Mapa da flash (8 MB)

```
0x000000  bootloader Órbita        (≤ 32 KB)
0x008000  tabela de partições
0x009000  nvs        64 KB         GLIDE, vínculo Bluetooth, calibração de rádio
0x019000  otadata     8 KB         qual app roda no próximo esp_restart()
0x020000  factory     3 MB         ► o Órbita
0x320000  glide     1,5 MB         ► GLIDE original (GLIDE.bin ~0,95 MB)
0x4A0000  orbitafs  3,3 MB         ► flash interna (LittleFS): notas, arquivos, /.orbita
0x7F0000  coredump   64 KB
```

O `orbita-full.bin` vai de 0x0 até o fim do GLIDE. Gravá-lo **não toca** a `orbitafs`, então notas e ajustes sobrevivem a atualizações. O `orbita-os.bin` (0x20000) atualiza só o sistema.

## 3. Boot

```
     reset por software (esp_restart)        ligar / BtnRst / USB / WDT
                 │                                      │
                 ▼                                      ▼
     partição do otadata (GLIDE)                 factory = Órbita
```

O Órbita procura a assinatura `ORBITA-BOOT-1` nos primeiros 32 KB da flash. Sem ela, a barra de status mostra **BL!** e o sistema avisa antes de abrir o GLIDE, porque aí o BtnRst não volta.

## 4. Apps

| App | Atalho | O que faz |
|---|---|---|
| **GLIDE** | `g` | abre o GLIDE embutido |
| **Apresentação** | `a` | teclado Bluetooth LE + USB para passar slides; cronômetro / regressivo com alertas |
| **Notas** | `n` | notas `.txt` em `/orbita/notas` (SD; sem cartão, flash interna) |
| **Arquivos** | `f` | SD e flash interna: editar texto, copiar/recortar/colar entre volumes, renomear, pastas, apagar |
| **Rede** | `r` | servidor web com gerenciador de arquivos; usuário `orbita` + PIN; Wi‑Fi de casa ou rede própria `Orbita-XXXX` com QR code |
| **USB** | `u` | cartão SD como pendrive |
| **Wi‑Fi** | `w` | procurar, conectar (lembra 5 redes), esquecer, rede própria |
| **Ajustes** | `j` | brilho, escurecer, cor, sons, abrir o GLIDE ao ligar, reiniciar |
| **Sistema** | `s` | bootloader, RAM, armazenamento, rede, GLIDE, Bluetooth, partições |

### Controle de apresentação

- **Bluetooth:** `BLE HID` com o nome "Orbita Apresentador". O pareamento é "Just Works" com vínculo, então o computador reconecta sozinho. O Bluetooth só liga com o app aberto e para de anunciar ao sair.
- **USB:** teclado TinyUSB, junto com o disco do modo pendrive e a serial. Só no build padrão (`cardputer-adv`).
- **Teclas enviadas:** setas ← → (ou PageUp/PageDown, tecla `k`), F5, Shift+F5, Esc, B (tela preta) e W (tela branca). Funcionam em PowerPoint, Google Slides, Keynote e LibreOffice Impress.
- **Tempo:** começa sozinho no primeiro slide. No regressivo há bipes a 5 min do fim, a 1 min e no zero; depois disso o número pisca com "+".
- A tela não escurece enquanto o app está aberto (`App::keepAwake()`).

## 5. Código

```
src/
├── main.cpp
├── core/
│   ├── Kernel        pilha de apps, laço, energia, abrir o GLIDE, barra de status
│   ├── App.h         classe base (render, onKey, update, keepAwake...)
│   ├── Input, UI, Dialogs, Theme
│   ├── Storage       volumes SD + flash (orbitafs), copiar/mover/apagar recursivo
│   ├── KvFile        "chave<TAB>valor" na flash: ajustes e redes Wi-Fi
│   ├── Settings      ajustes em /.orbita/ajustes
│   ├── Glide         partição do GLIDE: presente? abrir
│   ├── Hid           teclado Bluetooth LE + USB
│   ├── Net           Wi-Fi não-bloqueante
│   └── UsbDrive      disco USB (MSC) lendo setores do SD
└── apps/
    ├── HomeApp, PresenterApp, NotesApp, EditorApp, FilesApp
    ├── LanApp + WebUi.h, UsbApp, WifiApp, SettingsApp, SystemApp
third_party/glide/    GLIDE.bin oficial + LICENSE + NOTICES.txt
tools/flash.sh        gravar / gerar orbita-full.bin
tools/update-glide.sh atualizar o GLIDE embutido
```

## 6. Roteiro

- **Fonte com acentos** (Latin‑1) na interface
- **Ler pendrive no Cardputer** (USB host)
- **Relógio** pela internet (NTP) na barra de status e na apresentação
- **Apresentação:** vibrar o alerta pela caixa de som num padrão diferente, ponteiro laser virtual (mouse HID pelo IMU)
- Controle remoto IR, gravador de voz, nível (IMU), player de WAV

## 7. Checklist no aparelho

1. Gravar o `orbita-full.bin` → **Sistema** mostra "Bootloader: Orbita" e "GLIDE: gravado".
2. **GLIDE** abre; segurar `` ` `` reinicia no GLIDE; **BtnRst** volta ao Órbita.
3. **Apresentação**: parear "Orbita Apresentador" no notebook; `/` e `,` passam os slides; `i` inicia; `b` tela preta; regressivo de 1 min apita e pisca.
4. Apresentação pelo **cabo USB** (sem Bluetooth).
5. Mexer nos ajustes, abrir o GLIDE, voltar → ajustes e redes Wi‑Fi continuam.
6. **Notas**, **Arquivos**, **Rede**, **USB** como antes.

O que só o aparelho confirma: Bluetooth e USB HID funcionando ao mesmo tempo que o disco USB, a memória livre com Bluetooth e Wi‑Fi ligados juntos, e o GLIDE rodando a partir da partição nova.
