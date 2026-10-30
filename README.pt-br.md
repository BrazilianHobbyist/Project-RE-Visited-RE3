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
| Overlays (código que o jogo carrega do CD para menus e inimigos) | **Recompilados pelo mesmo recompilador** em shards nativos (`cache/`). Desde a v1.7.0 o cache é gerado **a partir do disco original**, não de sessões de jogo: todos os 17 overlays de menu e todos os 21 módulos de código de inimigos, num total de 59 imagens ([aot/README.md](aot/README.md)). O que ainda faltar roda num interpretador pequeno na primeira vez e é compilado em segundo plano pelo kit incluído (`overlay_toolchain/`: Python embutido + TinyCC). |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulados em software** pelo runtime do PSXRecomp (renderização via OpenGL/SDL3), como em outros projetos de recompilação estática. |
| BIOS | OpenBIOS (MIT, do PCSX-Redux), incluída. A animação de boot é pulada. |

**Medido, não suposto:** cada execução grava no `psx_last_run_report.json` (`interp_hist`) quanto código foi interpretado.

- Com o cache da v1.6.1, cerca de 20 minutos de jogo em quatro sessões (jogo novo, delegacia, portas, escadas, combate, inventário, baú, save) ficaram em no máximo **0,03%–0,15%** do trabalho da CPU emulada.
- Com o cache gerado a partir do disco, uma sessão na delegacia, com zumbis e portas, mediu **0,0061%**, com 17 instruções no código dos inimigos. O que resta é código da BIOS na RAM.

As releases anteriores (até a v1.6.0) saíram sem o cache de overlays e sem o kit, então o código das salas rodava interpretado na máquina dos jogadores. A v1.6.1 corrigiu isso com um cache das sessões de jogo do mantenedor. A v1.7.0 o substitui por um cache gerado a partir do disco.

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
| Analógico moderno relativo à tela (`modern_controls`) | Precisa de teste. O analógico segue os eixos fixos de cada câmera, como o controle alternativo do RE HD Remaster. Na v2.1.0, numa troca de câmera a direção atual continua até você soltar o analógico ou girá-lo mais de 45°; num corte para uma câmera quase oposta, só soltar encerra. Depois a direção passa à nova câmera suavemente (~0,2 s). O direcional digital usa sempre o controle tanque. Desligado por padrão |
| Portas rápidas (`fast_doors`) | Funciona. Pula a animação da porta, e a sala seguinte carrega na velocidade máxima do PC atrás da tela preta (a porta da sala do save caiu de ~3,8 s para ~1 s). A velocidade do jogo não muda |
| Pular cenas em tempo real com Select (Esc/Tab no teclado) | Funciona (usa o pulo do próprio jogo) |
| Pular FMV | Funciona; alguns FMVs só podem ser pulados depois de certo ponto, como no jogo original |
| Abertura rápida (animação da BIOS pulada, impressão digital do disco guardada) | Funciona |
| Saves em memory card (`saves/*.mcd`, formato padrão do PS1) | Funciona |
| Áudio | Funciona; a mixagem experimental (`spu_hq`) pode soar estourada e vem desligada |
| Cheats (opções no launcher + F6/F7/F8) | Funcionaram nos testes anteriores do mantenedor; não retestados desde a v1.6.0 |
| Resolução interna / supersampling, 16:9 esticado ou 4:3 | Precisa de teste |
| 16:9 Ampliado (`ws_layers`, opção Imagem no launcher) | Funciona nas sessões do mantenedor. Novo na v2.0.0: a cena 3D preenche o 16:9 sem distorção. Um pan vertical suave acompanha a Jill: fica parado enquanto ela está na faixa central, acelera e desacelera suavemente e já começa enquadrado a cada troca de câmera. Textos, avisos e a pausa ficam no tamanho original. Telas 2D (inventário, mapa, arquivos, menus), FMV e LIVE SELECTION ficam em 4:3. Algumas salas perdem um pouco do teto ou do chão. Desligado por padrão |
| Vibração | Precisa de teste |
| Texturas com correção de perspectiva | Precisa de teste |
| Fonte nítida (opção do launcher) | Funciona. Novo na v2.0.0: a primeira partida com a opção extrai a fonte do seu próprio jogo, e o launcher a amplia 4x (EPX). Nada do jogo vai no pacote |
| Correção geométrica PGXP | Precisa de teste. Elimina o tremor. Novo na v2.1.0: as juntas dos modelos (rosto, nuca, axila) ficam fechadas: o PGXP acompanha a solda que o próprio jogo faz entre as partes do modelo, inclusive a profundidade, e a expansão de emendas do PSXRecomp cobre as partes sem vértice em comum. Desligada por padrão |
| Velocidade do texto de introdução | Corrigida segundo o mantenedor para a v2.0.0; precisa de teste |
| Movimento suave (quadros intermediários, opção avançada do launcher) | Precisa de teste. O interpolador antigo foi removido (era placebo). Novo na v2.1.0, experimental: usa o Smooth motion do PSXRecomp: desenha a cena de novo entre dois quadros do jogo, com os modelos a meio caminho. A lógica e a velocidade do jogo não mudam. Medido num monitor de 144 Hz: ~72 imagens diferentes por segundo, contra ~36 sem a opção. Funciona junto com a fonte nítida e o 16:9 Ampliado. Problema conhecido: alguns artefatos em zumbis. Desligado por padrão |

Controles antigos que só funcionam via DirectInput: ative "Controles genéricos antigos (DirectInput)" no launcher. A opção vem desligada porque, em alguns PCs, atrasa a abertura em vários segundos; controles de Xbox, PlayStation, Switch e a maioria dos outros funcionam sem ela.

Relatos de bugs são bem-vindos. Inclua o `psx_last_run_report.json` da pasta do jogo e os passos para reproduzir.

---

## Início rápido

1. Baixe o zip mais recente em **Releases** e extraia.
2. Execute `Resident_Evil_3_Launcher.exe`.
3. Clique em **Instalar Jogo...** e selecione a imagem do seu disco de RE3 americano (`.cue`/`.bin`, setores brutos de 2352 bytes). Arquivos `.iso` de 2048 bytes são recusados: não contêm o áudio e o vídeo do jogo.
4. Escolha os ajustes (eles se salvam sozinhos; os técnicos ficam em **Opções avançadas...**) e clique em **JOGAR**.

## Controles

**Controle:** Analógico esquerdo / D-Pad = movimento (tanque por padrão; analógico relativo à tela no modo moderno) · A = ação · X = correr · Y = inventário · LB/RB = mirar/atirar · Back = mapa · Start = pausa
**Teclado:** Setas = mover · X = ação · Z = correr · A = inventário · Enter = start · Alt+Enter/F11 = tela cheia · F6/F7/F8 = cheats

---

## Compilando a partir do código-fonte

Requisitos: Windows 10/11 x64, Clang (ou MinGW-w64 GCC 12+), CMake 3.20+, Ninja, Python 3.11+ e a sua própria imagem do disco (`.cue` + `.bin`).

```powershell
git config --global core.longpaths true   # ou clone numa pasta curta: o submódulo tem caminhos longos
git clone --recurse-submodules https://github.com/BrazilianHobbyist/Project-RE-Visited-RE3.git
cd Project-RE-Visited-RE3
.\build.ps1 -Regen -DiscPath "C:\caminho\para\Resident Evil 3 - Nemesis (USA, Canada).cue"   # jogo
.\launcher\build.ps1    # launcher
```

O que o build faz:

- Aplica `patches/runtime/*.patch` (as mudanças do RE3, em ordem) sobre o commit fixado do PSXRecomp.
- Com `-Regen`, recompila o C do jogo a partir do seu disco em `generated/`. O código do jogo nunca está neste repositório.
- Aplica os dois `patches/re3_door_*.patch` ao C gerado.
- Compila com CMake e Ninja.

Nos builds seguintes, o `-Regen` pode ser omitido.

O build é reproduzível. O mesmo commit compilado com o mesmo compilador gera um executável e um launcher idênticos byte a byte. Isso foi verificado com um clone limpo contra o build de desenvolvimento, usando Clang 22.1.8. O `SOURCE_DATE_EPOCH` recebe a data do commit, e o link não grava timestamp.

O cache nativo de overlays também pode ser refeito só a partir do seu disco. Veja [aot/README.md](aot/README.md).

---

## Créditos

Veja [`CREDITS.md`](CREDITS.md).

* **PSXRecomp / RetroPortingToolkit**: framework de recompilação estática e runtime, por Matthew Stanley ("mstan") e colaboradores. Quase todo o trabalho de engine aqui é deles.
* **PCSX-Redux / OpenBIOS**: BIOS de código aberto.
* **PGXP**: iCatButler, simias, Pete Bernert.
* **Capcom**: Resident Evil 3: Nemesis (1999).
