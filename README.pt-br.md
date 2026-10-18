<p align="center">
  <img src="assets/banner.jpg" alt="Project RE: Visited - Resident Evil 3" width="100%">
</p>

# Project RE: Visited - Resident Evil 3

**Idiomas / Languages**: [English](README.md) | [**Português (Brasil)**](README.pt-br.md) | [Español](README.es.md)

---

**Port Nativo em C/C++ x86_64 e Recompilação Estática de Resident Evil 3: Nemesis (1999, PlayStation 1) para Windows**

---

## A Iniciativa Project RE: Visited

**Project RE: Visited** é uma iniciativa independente e de código aberto criada para restaurar, modernizar e preservar historicamente a lendária trilogia clássica de *survival horror* do PlayStation original no PC:
* **Resident Evil 3: Nemesis** (Lançamento Inaugural)
* **Resident Evil 2** (Planejado)
* **Resident Evil** (Planejado)

O objetivo é proporcionar a melhor experiência possível dos clássicos em hardware contemporâneo, combinando fidelidade artística impecável a avanços modernos de qualidade de vida (QoL), controles analógicos 3D em 360°, taxas de quadros ultra-fluidas e suporte a resoluções de até 4K — sem depender de emuladores tradicionais ou de versões antigas de PC limitadas.

---

## O que é Recompilação Estática Clean-Room?

Diferente de projetos de emulação tradicional ou de engenharia reversa que dependem de vazamentos de código-fonte, este projeto foi construído sob uma rigorosa metodologia de **Recompilação Estática Limpa (*Clean Room Static Recompilation*)**:

```
+-----------------------------------------------------------------------------------+
|                            CÓPIA LEGÍTIMA DO USUÁRIO                              |
|                    (CD-ROM original / Imagem ISO / BIN / CUE)                     |
+-----------------------------------------------------------------------------------+
                                          |
                        [Assistente de Instalação Nativo]
                  (Extração direta do executável SLUS_009.23)
                                          |
                                          v
+-----------------------+     +------------------------+     +----------------------+
|  Código de Máquina    |     |   Framework de         |     |  Código C Nativo     |
|  MIPS R3000A (PS1)    | --> |   Recompilação         | --> |  Moderno x86_64      |
|  (3.328 Funções RE3)  |     |   (PSXRecomp / Tool)   |     |  (Compilado em AOT)  |
+-----------------------+     +------------------------+     +----------------------+
                                                                        |
                                                                        v
+-----------------------------------------------------------------------------------+
|                   CAMADA DE ABSTRAÇÃO DE HARDWARE (HAL ABERTA)                    |
|      * Vídeo: OpenGL Moderno + PGXP (Sub-pixel & Perspectiva) + Shaders FXAA      |
|      * Áudio: SPU HQ 48kHz Resampling em Ponto Flutuante                          |
|      * Entrada: XInput / DirectInput / SDL2 com Suporte Analógico 360° Real       |
|      * Sistema: Reimplementação de BIOS Aberta (OpenBIOS / HLE Callbacks)         |
+-----------------------------------------------------------------------------------+
```

### 1. Tradução Direta de Instruções de Máquina
O binário compilado original (`SLUS_009.23`) tem suas instruções MIPS R3000A analisadas e traduzidas estaticamente para código C intermediário de alto desempenho. Cada rotina do jogo executa como código nativo de máquina x86_64 no processador do computador, sem o custo de desempenho de decodificação em tempo de execução (*JIT*) ou ciclos de interpretação de um emulador.

### 2. Nenhuma Linha de Código-Fonte Proprietário da Capcom
Nenhum código-fonte vazado, descompilação não autorizada de segredos industriais ou material protegido sob confidencialidade foi utilizado. O projeto é 100% de desenvolvimento limpo (*clean-room*), tratando o binário original como uma especificação estática de entrada.

### 3. Camada de Abstração de Hardware Aberta (HAL)
As chamadas de sistema, a GPU, o coprocessador geométrico GTE e o processador de som SPU são traduzidos em tempo de execução para APIs modernas e abertas (OpenGL, SDL2, OpenBIOS).

### 4. Distribuição Estritamente Livre de Ativos (*Zero-Asset Policy*)
**Este repositório NÃO armazena, NÃO hospeda e NÃO distribui nenhum arquivo de jogo, ROM, ISO, música, modelo ou textura.** Toda a mídia original necessária para jogar é extraída diretamente pelo usuário a partir de sua própria cópia legítima do jogo.

---

## Amparo Jurídico e Isenção de Responsabilidade

A engenharia reversa para fins estritos de **interoperabilidade de software e preservação digital** é amplamente protegida por marcos legais e precedentes jurídicos consolidados:

* **Estados Unidos**: *Digital Millennium Copyright Act* (DMCA), 17 U.S.C. § 1201(f) (*Interoperability Exception*), respaldado pelas decisões federais dos casos paradigmáticos *Sega Enterprises Ltd. v. Accolade, Inc.* (977 F.2d 1510) e *Sony Computer Entertainment, Inc. v. Connectix Corp.* (203 F.3d 596).
* **União Europeia**: Diretiva 2009/24/CE do Parlamento Europeu e do Conselho, de 23 de abril de 2009, Artigo 6º (*Descompilação com o objetivo indispensável de alcançar a interoperabilidade entre sistemas*).
* **Brasil**: Lei nº 9.609/1998 (Lei do Software), Artigo 6º, inciso I (*Não constitui ofensa aos direitos do titular a operação necessária para a obtenção de informações indispensáveis à interoperabilidade de programa criado de forma independente*).
* **Marcas Registradas**: *Resident Evil*, *Biohazard*, *Nemesis*, *Jill Valentine* são marcas comerciais da **Capcom Co., Ltd.**; *PlayStation* é marca da **Sony Interactive Entertainment Inc.** Seu uso aqui é estritamente referencial e descritivo (*nominative fair use*).
* **Termo Completo**: Consulte o documento jurídico detalhado em [**`CREDITS.md`**](CREDITS.md).

---

## Recursos e Melhorias Implementadas

Todas as opções listadas abaixo estão **100% implementadas, auditadas e funcionais** no projeto:

| Recurso | Descrição |
|---|---|
| **Assistente de Instalação Integrado** | Extração nativa de setores ISO 9660 Mode 2 Form 1. Detecta e extrai o executável original `SLUS_009.23` de imagens `.bin` / `.iso` em milissegundos sem ferramentas externas. |
| **Controles Analógicos Ágeis com Auto-Run** | Controle analógico no stick esquerdo com zona morta reduzida (0.26), corrida automática ao inclinar totalmente e rotação suave sem inversões de direção na troca de câmeras fixas. O D-Pad mantém 100% dos controles clássicos de tanque. |
| **Apresentação Autêntica a 30 FPS Nativos** | Renderização pura e estável a 30 FPS preservando a cadência original do PS1, máscaras 2.5D perfeitas e zero artefatos de ghosting ou sobreposição. |
| **Formatos de Tela Widescreen & Clássico** | Suporte nativo a 16:9 Widescreen Esticado (tela cheia para monitores modernos sem cortes de texto ou legendas) e 4:3 Clássico Original com barras laterais pretas autênticas. Configurável pelo Launcher ou em `settings.toml`. |
| **Pular Animação de Portas / Door Skip** | Pula as clássicas animações 3D de portas com transição rápida (~0,3s), fade in/out acelerado e carregamento de CD sem atraso artificial, preservando os diálogos. Configurável pelo Launcher ou em `settings.toml` (`fast_doors = true`). |
| **Vibração / Force Feedback (DualShock)** | Suporte autêntico a vibração em controles modernos (Xbox, DualShock 4, DualSense, etc.) para tiros, danos, explosões e passos de Nemesis via SDL GameController Force Feedback (intensidade calibrada em `settings.toml`). |
| **Correção Geométrica PGXP** | Elimina a instabilidade poligonal (*polygon jitter/wobble*) clássica do PS1 através de cálculo de vértices em ponto flutuante sub-pixel de alta precisão. |
| **Texturização com Correção de Perspectiva** | Mapeamento de textura com correção de perspectiva PGXP, removendo as distorções angulares e "dobras" de textura dos modelos 3D. |
| **Resolução Interna 3D Escalável** | Renderização interna de malhas poligonais em alta definição (1080p, 1440p e 4K Ultra HD) via supersampling (SSAA). |
| **Anti-Aliasing FXAA** | Filtro pós-processamento suave para bordas poligonais limpas e sem serrilhados perceptíveis. |
| **Pular FMVs / Cutscenes (Auto-Skip)** | Pule instantaneamente apresentações e cutscenes em vídeo FMV (MDEC / XA) pressionando a tecla de ação ou ativando a opção no Launcher. |
| **Áudio SPU de Alta Fidelidade (48 kHz)** | Resampling de áudio ADPCM a 48.000 Hz em ponto flutuante via shadow engine de calibração dinâmica. |
| **Launcher Gráfico Multi-idioma** | Interface visual nativa em Win32 e GDI+ ultraleve com suporte dinâmico a Português (PT-BR), Inglês (EN) e Espanhol (ES). |
| **Seletor de Cheats & Modificadores** | Menu dedicado e 100% reversível no Launcher: Vida Infinita (God Mode), Munição Infinita, Fitas de Salvamento Infinitas, Todas as Armas no Baú, Desbloqueio de The Mercenaries, Boutique & Trajes, e Todos os Epílogos. Com atalhos em tempo real no jogo. |
| **Memory Cards Preservados** | Sistema de salvamento padrão PS1 gravado em arquivos `.mcd` binários reais (`saves/card1.mcd`), totalmente compatíveis com emuladores reais e hardware de console. |

---

## Esclarecimento de Escopo (O que este projeto NÃO faz)

Para manter a integridade técnica, estabilidade e fidelidade histórica do jogo, delimitamos com clareza o que **não** faz parte deste projeto:

1. **Integração Segura de Memória Nativa (Sem Corrupção de Saves)**: O seletor de cheats atua diretamente nas flags nativas da Capcom (`0x800D1F32` / Clear Mode) e monitoramento seguro de buffers ativos. Não quebra slots de inventário, não causa itens fantasmas e é 100% reversível (desmarque a qualquer momento para jogar a campanha original clássica pura).
2. **Sem Texture Packs**: A fidelidade visual é aprimorada matematicamente via PGXP e SSAA sobre as texturas autênticas de 1999. O jogo não injeta texturas artificiais por substituição que alterem o tom sombrio concebido pelos artistas da Capcom.
3. **Sem Câmera Livre 3D nos Cenários Fixos**: Os cenários de *Resident Evil 3* são pinturas bidimensionais pré-renderizadas em 4:3 com ângulos de câmera dramáticos calculados para gerar tensão. A apresentação visual respeita a arte original sem quebrar a geometria invisível de colisões do jogo.

---

## Como Jogar e Instalar (Guia Rápido)

### Passo 1: Executar o Launcher
Abra a pasta do projeto (ou o pacote de distribuição baixado na aba Releases) e execute:
```
Resident_Evil_3_Launcher.exe
```

### Passo 2: Assistente de Instalação (Zero Fricção)
Se for sua primeira inicialização, o Launcher informará `[!] STATUS: Instalação Pendente`.
1. Clique em **`Instalar Jogo...`**.
2. **BIOS de Código Aberto (OpenBIOS)**: A **OpenBIOS** (reimplementação limpa sob licença MIT do PCSX-Redux) já vem **embutida e pré-configurada por padrão**, dispensando qualquer busca, download ou configuração de arquivo de BIOS para jogar.
3. No campo **Imagem do Jogo**, selecione seu arquivo `.bin`, `.cue` ou `.iso` do CD de *Resident Evil 3*.
4. Clique em **`Iniciar Instalação & Extração`**. O Launcher validará os arquivos e **extrairá o executável `SLUS_009.23` diretamente da imagem do disco** em instantes.

### Passo 3: Configurar e Jogar
1. Ajuste a proporção de tela (**16:9** ou **4:3**), resolução interna 3D (até **4K**), modo de controle (**Moderno 3D** ou **Clássico Tanque**) e filtros (PGXP e FXAA).
2. Clique em **`JOGAR RESIDENT EVIL 3`**!

---

## Controles

### Controle Xbox (XInput / Gamepad Moderno)
* **Analógico Esquerdo**: Movimentação 360° analógica (Zona morta 0.26 / Auto-run com inclinação total)
* **D-Pad**: Movimentação clássica tipo tanque (Tank Controls)
* **A**: Ação / Confirmar / Atirar / Pular FMVs
* **B**: Cancelar / Correr
* **X**: Correr
* **Y**: Inventário
* **LB / LT**: Mirar / Trocar de Alvo
* **RB / RT**: Atirar / Mirar Secundário
* **Back**: Mapa
* **Start**: Pausa

### Teclado
* **Setas**: Movimentação
* **X**: Ação / Confirmar / Pular FMVs
* **S**: Cancelar / Correr
* **Z**: Correr
* **A**: Inventário
* **Enter**: Start
* **Alt + Enter** ou **F11**: Alternar Tela Cheia / Janela
* **F6**: Alternar Vida Infinita (God Mode) Ligado/Desligado
* **F7**: Alternar Munição Infinita Ligado/Desligado
* **F8**: Recarregar Arma e Munições ao Máximo

---

## Salvamento (Memory Cards)

Os saves são gravados nos arquivos binários padrão do PlayStation:
* `saves/card1.mcd` (Slot 1)
* `saves/card2.mcd` (Slot 2)

Você pode migrar livremente seus saves de e para emuladores como DuckStation e Mednafen, ou dispositivos de hardware original do PS1.

---

## Instruções de Compilação (Para Desenvolvedores)

Se você deseja compilar o código-fonte manualmente:

### Pré-requisitos
* Windows 10/11 (64-bit)
* Compilador C/C++: MinGW-w64 (GCC 12+), Clang ou MSVC
* CMake 3.20+ e Ninja
* Python 3.8+

```powershell
# 1. Compilar o executável nativo do jogo:
.\build.ps1

# 2. Compilar o Launcher gráfico (Win32/GDI+):
.\launcher\build.ps1
```

---

## Créditos & Reconhecimento

Consulte o documento completo [**`CREDITS.md`**](CREDITS.md) para a lista exaustiva de todas as fontes, ferramentas e termos legais.

* **Capcom Co., Ltd.** — Pela criação da obra original *Resident Evil 3: Nemesis* (1999).
* **Sony Interactive Entertainment** — Plataforma de hardware PlayStation (PS1 / MIPS R3000A).
* **PSXRecomp (RetroPortingToolkit)** — Framework de recompilação estática por Matthew Stanley ("mstan").
* **Silent Hill (PS1) Decompilation & Native PC Port** — Pela inspiração fundamental em preservação de survival horror (Vatuu, shdecompilations, SlickAmogus e comunidade).
* **N64: Recompiled / Zelda64: Recompiled** — Pioneirismo de engenharia e recompilação por Mr-Wiseguy.
* **Ghidra SRE (NSA)** — Mapeamento de 3.328 funções do binário original.
* **PCSX-Redux & OpenBIOS** — BIOS aberta sob licença MIT.
* **Beetle PSX / Mednafen / Libretro** — Oráculo comparativo de ciclos de hardware.
* **DuckStation** — Referência técnica e de compatibilidade por stenzek.
* **PGXP** — Correção sub-pixel e perspectiva por iCatButler, simias, Pete Bernert.
* **SDL2 & OpenGL** — Gerenciamento de janelas, controles modernos e renderização gráfica 3D.
* **TinyCC (TCC)** — Sharding de overlays AOT por Fabrice Bellard.
