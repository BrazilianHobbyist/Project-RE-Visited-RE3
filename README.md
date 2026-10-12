# Project RE: Visited - Resident Evil 3 (Core & Recompiler)

**Recompilação Estática Clean-Room e Port Nativo em C/C++ x86_64 de Resident Evil 3: Nemesis (PS1)**  
Lançamento inaugural da iniciativa **Project RE: Visited**, dedicada à restauração, modernização e preservação histórica da trilogia clássica original de PlayStation no PC (Resident Evil 1, Resident Evil 2 e Resident Evil 3).

---

## 🔬 Arquitetura de Recompilação Estática Clean-Room

Este repositório contém a infraestrutura de recompilação estática, patches de qualidade de vida (QoL), camada de abstração de hardware (HAL) e ferramentas de build que traduzem o binário original de PlayStation 1 para um executável nativo moderno de 64 bits para Windows.

```
                              +---------------------------------------+
                              |   Binário Original SLUS_009.23 (PS1)  |
                              |        (MIPS R3000A Machine Code)     |
                              +---------------------------------------+
                                                  |
                    +-----------------------------+-----------------------------+
                    |                                                           |
                    v                                                           v
     +------------------------------+                            +------------------------------+
     |   Análise Estática Ghidra    |                            |    PSXRecomp Toolchain       |
     |   (seeds/ghidra_funcs.txt)   |                            |    (Tradução MIPS -> C)      |
     |   3.328 Funções Identificadas|                            |    (Controle de Fluxo/Salto) |
     +------------------------------+                            +------------------------------+
                    |                                                           |
                    +-----------------------------+-----------------------------+
                                                  |
                                                  v
                               +-------------------------------------+
                               |   Código C Nativo & Overlays AOT    |
                               |    (Compilado com MinGW / GCC / TCC)|
                               +-------------------------------------+
                                                  |
                                                  v
                               +-------------------------------------+
                               |   Runtime HAL (Hardware Abstraction)|
                               |   * Gráficos: OpenGL + PGXP + FXAA  |
                               |   * Som: SPU HQ 48kHz Resampling    |
                               |   * Input: SDL2 / XInput Analógico  |
                               |   * BIOS: OpenBIOS (MIT Clean-Room) |
                               +-------------------------------------+
                                                  |
                                                  v
                               +-------------------------------------+
                               | Resident_Evil_3__Nemesis_Recompiled |
                               +-------------------------------------+
```

### Princípios Técnicos Fundamentais:
1. **Zero Leaked Code**: Nenhum código-fonte vazado ou descompilação não autorizada da Capcom foi utilizado. O processo baseia-se puramente na análise estática de instruções de máquina MIPS R3000A.
2. **Desempenho Nativo sem JIT**: O código é traduzido previamente (AOT - *Ahead of Time*) em C e compilado para x86_64 nativo, executando diretamente na CPU sem o overhead de emulação interpretada ou recompilação JIT em tempo de execução.
3. **OpenBIOS Integration**: A camada de chamadas de sistema (syscalls) opera através da reimplementação aberta e limpa OpenBIOS (PCSX-Redux), tornando o runtime independente de BIOS proprietárias da Sony.
4. **Política Livre de Ativos (*Zero-Asset Policy*)**: O repositório não inclui nenhum asset protegido por direitos autorais (sem ROMs, ISOs, áudio, vídeos, modelos ou texturas). O usuário fornece seus próprios arquivos extraídos de sua mídia legítima.

---

## ⚖️ Marco Legal e Precedentes

A metodologia de engenharia reversa limpa para garantia de interoperabilidade tecnológica possui proteção explícita no direito internacional e comparado:
* **EUA**: Seção 1201(f) do DMCA (*Interoperability Reverse Engineering Exception*); precedentes federais *Sega v. Accolade* (977 F.2d 1510) e *Sony v. Connectix* (203 F.3d 596).
* **União Europeia**: Diretiva 2009/24/CE do Parlamento Europeu e do Conselho, Artigo 6º.
* **Brasil**: Artigo 6º, inciso I da Lei nº 9.609/1998 (Lei do Software).
* Consulte o documento [**`CREDITS.md`**](../CREDITS.md) para todos os termos jurídicos e créditos detalhados.

---

## ✨ Recursos Implementados e Auditados

- 🕹️ **Controles Modernos 3D (360°)**: Hook nativo no subsistema de entrada que calcula o vetor direcional analógico relativo à câmera atual, com zona morta ajustada (0.26) e ativação de corrida automática ao ultrapassar 82% de deflexão. O D-Pad mantém 100% dos controles clássicos de tanque.
- ⚡ **Interpolação Poligonal Temporal**: Amostragem de quadros com interpolação linear contínua (`linear blend` + `flip-aware source`) que sincroniza as malhas 3D à taxa do monitor (60Hz, 120Hz, 144Hz, 165Hz, 240Hz ou Auto-Sync).
- 💎 **Precisão Sub-pixel PGXP**: Coordenadas de vértices em ponto flutuante que eliminam o tremor poligonal (*jitter*) do PS1 original.
- 🎯 **Mapeamento de Perspectiva PGXP**: Correção de perspectiva em texturas 3D, removendo deformações visuais em paredes e pisos.
- 🖥️ **Widescreen 16:9 & Clássico 4:3**: Apresentação de tela adaptada para telas modernas com modos Tela Cheia (Sem Bordas), Tela Cheia Exclusiva e Janela. *(Esclarecimento: os cenários pré-renderizados 2D são preservados em sua proporção estética sem quebra artificial de câmeras).*
- 📐 **Resolução Interna 3D até 4K UHD**: Renderização interna de modelos poligonais com supersampling (SSAA) escalável (1080p, 1440p, 4K).
- 🪄 **Anti-Aliasing FXAA**: Pós-processamento de bordas poligonais em GLSL shader.
- ⏩ **Pular FMVs / Cutscenes (Auto-Skip)**: Bypass direto nos loops de decodificação MDEC e áudio XA ao pressionar a tecla de ação.
- 🔊 **Áudio SPU HQ 48kHz**: Resampling em ponto flutuante com interpolação cúbica de 4 pontos.
- 💾 **Saves Nativos em Memory Cards PS1**: Leitura e escrita binária em `saves/card1.mcd` e `card2.mcd`.

> ℹ️ **Esclarecimento de Escopo**: O projeto não inclui trainers ou modificações diretas de memória de inventário (pois causam corrupção de salvamentos na máquina de estados de RE3) e não utiliza pacotes externos de substituição artificial de texturas por IA, preservando a arte original da Capcom de 1999.

---

## 🛠️ Pré-requisitos de Compilação

- **Sistema Operacional**: Windows 10/11 (64-bit)
- **Compilador C/C++**: MinGW-w64 (GCC 12+), Clang ou MSVC (Visual Studio 2022)
- **Build System**: [CMake 3.20+](https://cmake.org/) e [Ninja](https://ninja-build.org/)
- **Linguagem Auxiliar**: [Python 3.8+](https://www.python.org/)
- **Cópia Legal do Jogo**: Imagem de CD de *Resident Evil 3: Nemesis* (SLUS-00923) para extração de dados

---

## 📁 Estrutura do Diretório

```
RE3Recomp/
├── launcher/               ← Código-fonte em Win32/GDI+ do Launcher com instalador ISO 9660
├── patches/                ← Patches customizados de interpolação, câmera e controles
├── psxrecomp/              ← Framework PSXRecomp (runtime HAL e recompiler estático)
├── seeds/                  ← Tabela de 3.328 funções mapeadas (ghidra_funcs.txt)
├── tools/                  ← Ferramentas de análise e exportação de dados
├── game.toml               ← Configuração central de símbolos e recompilação
├── CMakeLists.txt          ← Build system CMake
├── build.ps1               ← Script de compilação automatizada do executável do jogo
└── README.md               ← Esta documentação técnica
```

---

## 💻 Instruções de Build

### 1. Compilar o Executável Principal do Jogo
Abra o PowerShell na pasta `RE3Recomp` e execute:
```powershell
.\build.ps1
```
O script configurará o CMake com o gerador Ninja e compilará o binário x86_64:  
`Resident_Evil_3__Nemesis_Recompiled.exe`

### 2. Compilar o Launcher com Instalador ISO 9660
Para compilar a interface gráfica nativa em C++/Win32:
```powershell
.\launcher\build.ps1
```
O executável gerado será:  
`Resident_Evil_3_Launcher.exe`

### 3. Distribuição e Pacote Final
Os arquivos finais compilados, junto com o arquivo de configuração e a pasta `cache/` de overlays compilados, podem ser distribuídos no diretório autônomo `ResidentEvil3-PCPort`.

---

## 🎮 Mapeamento de Controles (XInput / Gamepad)

| PS1 Original | Gamepad Xbox | Função em RE3 |
|---|---|---|
| **Analógico Esquerdo** | Stick Analógico Esquerdo | Movimentação 360° Analógica / Auto-run (>82%) |
| **D-Pad** | D-Pad | Controles Tanque Clássicos (Girar / Andar) |
| **× (Cross)** | A | Ação / Confirmar / Atirar / Pular FMV |
| **○ (Circle)** | B | Cancelar / Correr |
| **□ (Square)** | X | Correr |
| **△ (Triangle)** | Y | Inventário |
| **L1** | LB | Mirar Alvo Principal |
| **R1** | RB | Atirar |
| **L2** | LT | Trocar Alvo |
| **R2** | RT | Trocar Alvo Secundário |
| **Select** | Back | Mapa |
| **Start** | Start | Pausa |

---

## 📜 Créditos e Referências

Consulte o documento completo [**`CREDITS.md`**](CREDITS.md) para atribuições completas.

* **Capcom Co., Ltd.** — Obra original *Resident Evil 3: Nemesis* (1999).
* **Sony Interactive Entertainment** — Plataforma PlayStation original (PS1 / MIPS R3000A).
* **PSXRecomp (RetroPortingToolkit)** — Framework de recompilação estática por Matthew Stanley ("mstan").
* **Silent Hill (PS1) Decompilation & Native PC Port** — Inspiração de preservação (Vatuu, shdecompilations, SlickAmogus e comunidade).
* **N64: Recompiled / Zelda64: Recompiled** — Pioneirismo técnico em recompilação estática limpa por Mr-Wiseguy.
* **Ghidra SRE (NSA)** — Análise e mapeamento de 3.328 rotinas do binário original.
* **PCSX-Redux & OpenBIOS** — BIOS de código aberto 100% limpa (clean-room).
* **DuckStation & Beetle PSX** — Oráculos de precisão e compatibilidade.
* **PGXP** — Correção sub-pixel e perspectiva por iCatButler, simias, Pete Bernert.
* **SDL2 & OpenGL** — Camada de hardware, janelas e aceleração 3D.
