# Órbita OS

App para o **M5Stack Cardputer ADV** que roda dentro do **[M5Launcher](https://github.com/bmorcelli/Launcher)**. O Launcher é o motor de boot (você escolhe entre o Órbita, o [GLIDE](https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV) e outros firmwares) e já cuida de arquivos por USB e pela rede. O Órbita traz o que o Launcher não tem: **controle de apresentação com cronômetro**, **notas** e um **gerenciador de arquivos** no próprio aparelho.

Detalhes do desenho em [`docs/ARQUITETURA.md`](docs/ARQUITETURA.md).

## Instalar pelo M5Launcher

1. Baixe o **[`orbita-launcher.bin`](../../releases/latest/download/orbita-launcher.bin)** (página [Releases](../../releases/latest)).
2. Copie para o cartão SD (pelo próprio Launcher: **WUI** no navegador ou modo USB).
3. No Launcher: **SD** → `orbita-launcher.bin` → **Install**.

O Launcher lê a tabela de partições do arquivo e cria também a **flash interna** do Órbita (`orbitafs`). Para trocar de firmware, ligue o Cardputer ou aperte **BtnRst** e aperte uma tecla na tela do Launcher.

> O `orbita-launcher.bin` também pode ser gravado sozinho em `0x0` (ESP Web Flasher), sem o Launcher, mas isso apaga o Launcher da flash.

## O que tem

| App | Atalho | O que faz |
|---|---|---|
| **Apresentação** | `a` | Passa slides por **Bluetooth** ou **cabo USB** (o Cardputer vira teclado) + cronômetro ou tempo regressivo com alertas |
| **Notas** | `n` | Bloco de notas com editor de texto (no SD; sem cartão, na flash interna) |
| **Arquivos** | `f` | SD e flash interna: editar texto, copiar/recortar/colar, renomear, pastas, apagar |
| **Ajustes** | `j` | Brilho, escurecer tela, cor de destaque, sons |
| **Sistema** | `s` | Memória, bateria, armazenamento, Bluetooth, partições |

## Controle de apresentação

1. Abra **Apresentação**. No computador, pareie com **Orbita Apresentador** por Bluetooth (só na primeira vez) ou ligue o cabo USB‑C.
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

O tempo começa sozinho no primeiro slide. No regressivo, o número fica amarelo nos últimos 5 minutos, vermelho no último e pisca quando passa do tempo, com bipes em cada etapa. A tela não escurece com o app aberto.

## Teclas gerais

`,` `/` navegar · `;` `.` cima/baixo · `Enter` abrir · `` ` `` voltar · `Tab` opções · letra = atalho do app

No editor de texto as letras digitam normalmente; **fn** + `; , . /` move o cursor e **ctrl+s** salva.

## Compilar

A cada push, o **GitHub Actions** compila e publica em **Releases → latest**. No computador:

```bash
pip install platformio esptool
pio run -e cardputer-adv     # compila
tools/flash.sh --merge       # gera out/orbita-launcher.bin e out/orbita-app.bin
```

`cardputer-adv-sem-usb` compila sem o teclado USB (a apresentação fica só no Bluetooth).

## Licença

Código do Órbita: sugestão MIT. O GLIDE não faz mais parte deste repositório; instale-o pelo Launcher a partir do [repositório oficial](https://github.com/CHARL3X/GLIDE-Synth-Cardputer-ADV).
