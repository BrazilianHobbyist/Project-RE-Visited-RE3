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
| Overlays (código de cada sala/estágio carregado do CD em `0x80165000`) | **Interpretados** por padrão. Só são compilados para nativo em tempo de execução se houver um compilador C (`gcc`/`clang`) no `PATH`. Mesmo assim, a maioria das chamadas de overlay ainda passa pelo interpretador. |
| GPU, GTE, SPU, CD-ROM, MDEC, timers | **Emulados em software** pelo runtime do PSXRecomp (renderização via OpenGL/SDL2). |
| BIOS | OpenBIOS (MIT, do PCSX-Redux), incluída. |

Ou seja, é um híbrido: o código principal do jogo é recompilado, mas boa parte do que roda durante o gameplay é interpretado, e o hardware é emulado. Não é um "port nativo" no sentido de código descompilado e reescrito. Por isso características do PS1, como o tremido dos polígonos, continuam: a matemática de ponto fixo do GTE é reproduzida fielmente.

## Nota legal

O executável da release contém o código de máquina do jogo traduzido para x86-64. Você precisa possuir Resident Evil 3: Nemesis (SLUS-00923) e fornecer sua própria imagem de disco. Não são distribuídas imagens de disco, áudio, vídeo, texturas nem BIOS da Sony. Projeto de fã, sem fins comerciais; o PSXRecomp é licenciado sob PolyForm Noncommercial 1.0.0. Resident Evil e nomes relacionados são marcas da Capcom; PlayStation é marca da Sony Interactive Entertainment.

---

## Funcionalidades

Legenda: **Funciona** = confirmado · **Precisa de teste** = implementado, não reverificado após mudanças recentes · **Problemas conhecidos** = relatado como quebrado ou parcial.

| Funcionalidade | Status |
|---|---|
| Assistente de instalação do launcher (extrai `SLUS_009.23` do seu `.bin`/`.cue`/`.iso`) | Funciona |
| Resolução interna / supersampling, 16:9 esticado ou 4:3 | Precisa de teste |
| Controles analógicos modernos com auto-run (`modern_controls`) | Problemas conhecidos: quebrado na v1.4.x, restaurado na v1.5.1, precisa de reteste |
| Portas rápidas (`fast_doors`) | Problemas conhecidos: pular a cutscene após uma porta pode travar o jogo |
| Pular FMV | Precisa de teste |
| Vibração (SDL2) | Precisa de teste |
| Cheats (opções no launcher + F6/F7/F8) | Precisa de teste |
| Correção geométrica PGXP | Problemas conhecidos: sem efeito visível / deforma personagens; não recomendado |
| Texturas com correção de perspectiva | Precisa de teste |
| Áudio | Problemas conhecidos: estalos nas transições de sala |
| Velocidade do texto de introdução | Problemas conhecidos: texto avança rápido demais |
| 60 FPS / interpolação de quadros | **Removido.** Nunca funcionou de verdade (era placebo) |
| Saves em memory card (`saves/*.mcd`, formato padrão do PS1) | Precisa de teste |

Relatos de bugs são bem-vindos. Inclua o `psx_last_run_report.json` da pasta do jogo e os passos para reproduzir.

---

## Início rápido

1. Baixe o zip mais recente em **Releases** e extraia.
2. Execute `Resident_Evil_3_Launcher.exe`.
3. Clique em **Instalar Jogo** e selecione a imagem do seu disco de RE3 (`.bin`, `.cue` ou `.iso`).
4. Configure e clique em **JOGAR RESIDENT EVIL 3**.

## Controles

**Controle:** Analógico esquerdo = movimento (modo moderno) · D-Pad = controle tanque clássico · A = ação · X = correr · Y = inventário · LB/RB = mirar/atirar · Back = mapa · Start = pausa
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
