# Órbita OS — arquitetura e roteiro

O Órbita é o **firmware principal** do **M5Stack Cardputer ADV**: o núcleo que amarra tudo. Ele tem funções de conveniência próprias (notas, arquivos, rede, USB) e um menu para **trocar de firmware** quando o usuário quer um app grande, como o jogo **Hazke** ou o synth **GLIDE**.

Hardware: ESP32‑S3FN8 a 240 MHz, 8 MB de flash, **sem PSRAM** (~512 KB de SRAM), tela ST7789V2 240×135, teclado 56 teclas (TCA8418), áudio ES8311, IMU BMI270, IR, microSD.

## 1. Dois tipos de app

| Tipo | Onde roda | Exemplos | Troca |
|---|---|---|---|
| **Apps do Órbita** (nativos) | dentro do próprio firmware | Notas, Arquivos, Rede, USB, Wi‑Fi, Ajustes, Sistema | instantânea |
| **Firmwares completos** (slots) | uma partição própria, o chip reinicia neles | Hazke, GLIDE | ~1 s; **BtnRst** volta ao Órbita |

Por que o Hazke e o GLIDE ficam em slots:

| Programa | Licença | Código | Como entra |
|---|---|---|---|
| **Hazke** (therezor) | MIT | aberto, sketch Arduino | binário no slot (porte nativo é possível no futuro) |
| **GLIDE** (CHARL3X) | PolyForm Noncommercial 1.0 | o repositório publica o `GLIDE.bin`, não o código | sempre binário no slot |

> Uso pessoal e não comercial do GLIDE é livre. O Órbita não embute nem redistribui o GLIDE: o usuário copia o `GLIDE.bin` oficial para o cartão.

## 2. Mapa da flash (8 MB)

```
0x000000  bootloader Órbita        (≤ 32 KB)
0x008000  tabela de partições
0x009000  nvs        20 KB         ajustes, redes Wi-Fi, nomes dos slots (e dados dos apps)
0x00E000  otadata     8 KB         qual partição roda no próximo esp_restart()
0x010000  factory   1,875 MB       ► o Órbita
0x1F0000  app0       2,5 MB        ► slot 1 (ex.: Hazke)
0x470000  app1       2,5 MB        ► slot 2 (ex.: GLIDE)
0x6F0000  spiffs      1 MB         LittleFS = "flash interna" (saves internos do Hazke também)
0x7F0000  coredump   64 KB
```

O cartão SD é o armazenamento principal de dados (notas, arquivos, `.bin` dos apps). Código só executa a partir da flash interna — por isso os firmwares completos precisam de um slot.

## 3. Como o boot funciona

O **bootloader de 2º estágio customizado** (`bootloader/`, baseado no exemplo `bootloader_override` do ESP‑IDF) decide pela causa do reset:

```
     reset por software (esp_restart)        ligar / botão RST / USB / WDT
     ou deep sleep                                      │
                 │                                      ▼
                 ▼                               partição factory
     partição escolhida no otadata               = Órbita
     = firmware do slot
```

* O Órbita abre um firmware com `esp_ota_set_boot_partition(slot)` + `esp_restart()`.
* O **BtnRst sempre volta ao Órbita**, sem o firmware saber de nada.
* Firmwares que se reiniciam sozinhos continuam neles mesmos (ex.: no GLIDE, segurar `` ` `` reinicia no GLIDE).
* Quando o próprio Órbita quer reiniciar, ele antes aponta o otadata para a factory.

O Órbita procura a assinatura `ORBITA-BOOT-1` nos primeiros 32 KB da flash. Sem ela, a barra de status mostra **BL!** e ele avisa antes de abrir um firmware.

## 4. Funções do Órbita

| App | O que faz |
|---|---|
| **Apps** | lista os slots: abrir (troca o firmware), substituir por outro `.bin`, renomear, apagar; instalar do SD |
| **Notas** | notas `.txt` em `/orbita/notas` (no SD; sem cartão, na flash interna); editor com cursor, rolagem, quebra de linha |
| **Arquivos** | SD e flash interna: abrir/editar texto, instalar `.bin`, copiar/recortar/colar (também entre volumes), renomear, nova pasta, novo arquivo, apagar pastas inteiras |
| **Rede** | servidor web: gerenciador de arquivos no navegador, enviar arquivos (arrastar e soltar), baixar, renomear, apagar, **instalar `.bin` num slot e abrir firmware pelo navegador**. Usuário `orbita` + PIN na tela. Funciona no Wi‑Fi de casa (`http://IP` ou `orbita.local`) ou criando a rede própria `Orbita-XXXX` — o QR code na tela conecta o celular |
| **USB** | o cartão SD vira pendrive no computador pelo USB‑C; o sistema não toca no cartão enquanto isso |
| **Wi‑Fi** | procurar redes, conectar (lembra até 5), esquecer, criar rede própria, desligar |
| **Ajustes** | brilho, escurecer tela, cor de destaque, abrir o último firmware ao ligar, sons, reiniciar |
| **Sistema** | bootloader, RAM, armazenamento, rede, slots, partições, bateria |

## 5. Camadas do código

```
src/
├── main.cpp              setup()/loop() → Kernel
├── core/
│   ├── Kernel            pilha de apps, laço, energia, instalar/abrir firmware, barra de status
│   ├── App.h             classe base dos apps nativos
│   ├── Input             teclado → eventos (setas ; , . /  Esc `), repetição de tecla
│   ├── UI                fontes, lista com rolagem, ícones vetoriais, quebra de texto
│   ├── Dialogs           Confirmar, Escolher, Digitar texto
│   ├── Storage           volumes SD + flash (LittleFS), copiar/mover/apagar recursivo
│   ├── SlotManager       slots: instalar de qualquer volume, abrir, apagar, renomear
│   ├── Net               Wi-Fi não-bloqueante: busca, conexão, redes salvas, ponto de acesso
│   ├── UsbDrive          disco USB (TinyUSB MSC) lendo setores do SD
│   ├── Settings / Theme  ajustes na NVS, paleta
└── apps/
    ├── HomeApp           carrossel: firmwares instalados + apps do Órbita
    ├── FirmwareApp       "Apps": gerenciar e trocar firmwares
    ├── NotesApp, EditorApp
    ├── FilesApp          gerenciador de arquivos local
    ├── LanApp + WebUi.h  servidor web e página do navegador
    ├── UsbApp, WifiApp, SettingsApp, SystemApp
```

**Tela:** tudo é desenhado num sprite de 240×135 (≈64 KB) e enviado de uma vez, sem piscar; só redesenha quando algo muda.

**Pilha de apps:** `push/pop` são aplicados depois do evento atual; diálogos são overlays.

### Modo USB: detalhe de compilação

O modo pendrive exige o USB em modo OTG (TinyUSB) — é o ambiente padrão `cardputer-adv`. A serial pelo USB continua funcionando. Se o computador não conseguir gravar sozinho nesse modo, desligue o Cardputer, segure **G0** e ligue o cabo. Existe o ambiente `cardputer-adv-sem-usb` (USB nativo do chip) caso prefira abrir mão do pendrive.

### Como escrever um app nativo

```cpp
class RelogioApp : public App {
 public:
  const char* title() const override { return "Relogio"; }
  bool animating() const override { return true; }
  void onKey(const KeyEvent& e) override {
    if (e.nav == Nav::Back) Kernel::get().pop();
  }
  void render(M5Canvas& c, const Rect& area) override {
    c.setFont(ui::title());
    c.drawString(String(millis() / 1000).c_str(), area.x + 10, area.y + 10);
  }
};
// Em HomeApp::rebuild():
// items_.push_back({"Relogio", "...", ui::Icon::Generic, 'R', -1,
//                   []{ Kernel::get().push(new RelogioApp()); }});
```

## 6. Teclas

| Tecla | Menus | Editor de texto |
|---|---|---|
| `,` `/` | ← → | digita `,` `/` |
| `;` `.` | ↑ ↓ | digita `;` `.` |
| **fn** + `; , . /` | — | move o cursor |
| `Enter` | abrir | nova linha |
| `` ` `` | voltar | sair (pergunta se salva) |
| `Tab` | menu de opções | dois espaços |
| `Del` | opções | apagar (fn+Del: para frente) |
| `ctrl`+`s` | — | salvar |
| `1`…`9` | abrir firmware do slot | — |
| Arquivos | `c` copiar · `x` recortar · `v` colar · `f` troca SD/flash · `r` atualizar | |
| **BtnRst** | dentro de um firmware: volta ao Órbita | |

## 7. Roteiro

### Feito
- [x] Bootloader Órbita, 2 slots de 2,5 MB, troca de firmware
- [x] Apps, Notas, Arquivos (SD + flash), Rede (navegador), USB (pendrive), Wi‑Fi, Ajustes, Sistema
- [x] Instalar e abrir firmware pelo navegador
- [x] Build automático no GitHub Actions

### Próximos
- **Fonte com acentos** (Latin‑1) — hoje a interface está sem acento
- **Pendrive no Cardputer** (USB host): ler um pendrive ligado ao Cardputer por OTG
- **Backup de saves por firmware**: copiar a flash interna para o SD ao trocar de firmware
- **Loja Órbita**: catálogo JSON apontando para as releases do Hazke e do GLIDE, download direto para o slot
- Relógio (NTP), controle remoto IR, gravador de voz, nível (IMU), player de WAV, scripts Lua/Berry no SD

## 8. Checklist de validação no hardware

1. Gravar com `tools/flash.sh` → **Sistema** diz "Bootloader: Orbita".
2. **USB**: aparece um disco no computador; copiar `hazke-v1.4.0-app.bin` e `GLIDE.bin` para `/orbita/apps/`; ejetar; sair.
3. **Apps** → instalar Hazke no slot 1 e GLIDE no slot 2.
4. Abrir o Hazke → **BtnRst** → volta ao Órbita. Abrir o GLIDE → segurar `` ` `` → reinicia no GLIDE → **BtnRst** → Órbita.
5. **Rede** no Wi‑Fi de casa: abrir o IP no celular, digitar o PIN, enviar um arquivo, instalar um `.bin`, abrir um slot pelo navegador.
6. **Rede** sem roteador ("rede própria"): escanear o QR, abrir `192.168.4.1`.
7. **Notas**: criar, editar com fn+setas, salvar, reabrir; sem cartão, criar uma nota na flash.
8. **Arquivos**: copiar uma pasta do SD para a flash e de volta; apagar a pasta.

O que só o hardware confirma: o código de reset do BtnRst no S3, o disco USB aparecendo com o TinyUSB, se o Hazke e o GLIDE cabem em 2,5 MB cada, e se o Órbita cabe em 1,875 MB.
