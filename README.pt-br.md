<p align="center">
  <img src="assets/banner.jpg" alt="Project RE: Visited - Resident Evil 3" width="100%">
</p>

# Project RE: Visited - Resident Evil 3

**Languages / Idiomas**: [English](README.md) | [**Português (Brasil)**](README.pt-br.md) | [Español](README.es.md)

---

**Port experimental para Windows de Resident Evil 3: Nemesis (PS1, SLUS-00923), construído sobre o framework de recompilação estática [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp).**

> **Status: em desenvolvimento / experimental.** Espere bugs. As notas de release anteriores e este README exageravam o que funciona; esta versão descreve o projeto como ele realmente é.

---

## Como funciona de verdade

| Parte | Como roda |
|---|---|
| Executável principal (`SLUS_009.23`, 3.328 funções) | **Recompilado estaticamente** MIPS → C → x86-64. Roda como código nativo. |
| Overlays (código que o jogo carrega do CD para salas, menus, inventário etc.) | **Recompilados pelo mesmo recompilador** em shards nativos (`cache/`). A release traz os shards das áreas jogadas até agora. Código que ainda não está no cache roda num interpretador pequeno na primeira vez, é registrado e compilado em segundo plano pelo kit incluído (`overlay_toolchain/`: Python embutido + TinyCC), e passa a rodar nativo a partir da próxima visita. |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulados em software** pelo runtime do PSXRecomp (renderização via OpenGL/SDL2), como em outros projetos de recompilação estática. |
| BIOS | OpenBIOS (MIT, do PCSX-Redux), incluída. A animação de boot é pulada. |

**Medido, não suposto:** em cerca de 20 minutos de jogo, em quatro sessões (jogo novo, delegacia, portas, escadas, combate, inventário, baú, save), as instruções interpretadas foram no máximo **0,03%–0,15%** do trabalho da CPU emulada; o resto rodou como código nativo. Cada execução grava esses números no `psx_last_run_report.json` (`interp_hist`).

As releases anteriores (até a v1.6.0) saíram sem o cache de overlays e sem o kit, então na máquina dos jogadores o código das salas rodava interpretado. Isso foi corrigido na v1.6.1.

Não é uma descompilação: o código do jogo é traduzido automaticamente, não reescrito, e o hardware é emulado. Por isso características do PS1, como o tremido dos polígonos, continuam: a matemática de ponto fixo do GTE é reproduzida fielmente.

## Nota legal

O executável da release e os shards de overlay em `cache/` contêm o código de máquina do jogo traduzido para x86-64. Você precisa possuir Resident Evil 3: Nemesis (SLUS-00923) e fornecer sua própria imagem de disco. Não são distribuídas imagens de disco, áudio, vídeo, texturas nem BIOS da Sony. Projeto de fã, sem fins comerciais; o PSXRecomp é licenciado sob PolyForm Noncommercial 1.0.0. Resident Evil e nomes relacionados são marcas da Capcom; PlayStation é marca da Sony Interactive Entertainment.

---

## Funcionalidades

Legenda: **Funciona** = confirmado · **Precisa de teste** = implementado, não reverificado após mudanças recentes · **Problemas conhecidos** = relatado como quebrado ou parcial.

| Funcionalidade | Status |
|---|---|
| Assistente de instalação do launcher (extrai `SLUS_009.23` do seu `.cue`/`.bin`) | Funciona |
| Controle clássico (tanque) | Funciona (padrão) |
| Analógico moderno relativo à tela (`modern_controls`) | Problemas conhecidos: funciona, mas fica impreciso em salas pequenas; desligado por padrão até ser refinado |
| Pular a animação das portas (`fast_doors`) | Funciona (transição em silêncio, com fade curto) |
| Pular cenas em tempo real com Select (Esc/Tab no teclado) | Funciona (usa o pulo do próprio jogo) |
| Pular FMV | Funciona; alguns FMVs só podem ser pulados depois de certo ponto, como no jogo original |
| Abertura rápida (animação da BIOS pulada, impressão digital do disco guardada) | Funciona |
| Saves em memory card (`saves/*.mcd`, formato padrão do PS1) | Funciona |
| Áudio | Funciona; a mixagem experimental (`spu_hq`) pode soar estourada e vem desligada |
| Cheats (opções no launcher + F6/F7/F8) | Funcionaram nos testes anteriores do mantenedor; não retestados na v1.6.1 |
| Resolução interna / supersampling, 16:9 esticado ou 4:3 | Precisa de teste |
| Vibração (SDL2) | Precisa de teste |
| Texturas com correção de perspectiva | Precisa de teste |
| Correção geométrica PGXP | Problemas conhecidos: deforma personagens; não recomendado |
| Velocidade do texto de introdução | Problemas conhecidos: texto avança rápido demais |
| 60 FPS / interpolação de quadros | **Removido.** Nunca funcionou de verdade (era placebo) |

Controles antigos que só funcionam via DirectInput: ative "Controles genéricos antigos (DirectInput)" no launcher. A opção vem desligada porque, em alguns PCs, atrasa a abertura em vários segundos; controles de Xbox, PlayStation, Switch e a maioria dos outros funcionam sem ela.

Relatos de bugs são bem-vindos. Inclua o `psx_last_run_report.json` da pasta do jogo e os passos para reproduzir.

---

## Início rápido

1. Baixe o zip mais recente em **Releases** e extraia.
2. Execute `Resident_Evil_3_Launcher.exe`.
3. Clique em **Instalar Jogo...** e selecione a imagem do seu disco de RE3 americano (`.cue`/`.bin`, setores brutos de 2352 bytes). Arquivos `.iso` de 2048 bytes são recusados: não contêm o áudio e o vídeo do jogo.
4. Configure e clique em **JOGAR RESIDENT EVIL 3**.

## Controles

**Controle:** Analógico esquerdo / D-Pad = movimento (tanque por padrão; analógico relativo à tela no modo moderno) · A = ação · X = correr · Y = inventário · LB/RB = mirar/atirar · Back = mapa · Start = pausa
**Teclado:** Setas = mover · X = ação · Z = correr · A = inventário · Enter = start · Alt+Enter/F11 = tela cheia · F6/F7/F8 = cheats

---

## Compilando a partir do código-fonte

Requisitos: Windows 10/11 x64, MinGW-w64 GCC 12+ ou Clang, CMake 3.20+, Ninja, Python 3.8+ e sua própria cópia de `SLUS_009.23`.

```powershell
git clone --recurse-submodules https://github.com/BrazilianHobbyist/Project-RE-Visited-RE3.git
.\build.ps1             # jogo
.\launcher\build.ps1    # launcher
```

As alterações no runtime em relação ao PSXRecomp original estão em `patches/`.

---

## Créditos

Veja [`CREDITS.md`](CREDITS.md).

* **PSXRecomp / RetroPortingToolkit**: framework de recompilação estática e runtime, por Matthew Stanley ("mstan") e colaboradores. Quase todo o trabalho de engine aqui é deles.
* **PCSX-Redux / OpenBIOS**: BIOS de código aberto.
* **PGXP**: iCatButler, simias, Pete Bernert.
* **Capcom**: Resident Evil 3: Nemesis (1999).
