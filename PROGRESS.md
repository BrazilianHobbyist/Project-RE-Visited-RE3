# Project RE: Visited — Resident Evil 3: Nemesis
## Relatório Consolidado de Auditoria, Memória e Progresso do Projeto

> **Data da Última Auditoria**: 17 de Outubro de 2026  
> **Status Geral**: Auditado, sincronizado e consolidado (**v1.5.0** / Build de 17/10/2026). As 3 cópias do executável do jogo e as 3 cópias do Launcher estão 100% idênticas (verificadas via SHA-256). Plano de governança persistente ativo em [`WORKFLOW.md`](WORKFLOW.md) e automatizado por [`tools/manage_project.ps1`](RE3Recomp/tools/manage_project.ps1).

---

## 1. Identificação Técnica do Jogo

| Parâmetro | Valor |
| :--- | :--- |
| **Jogo** | Resident Evil 3: Nemesis (1999) |
| **Plataforma Original** | Sony PlayStation 1 (MIPS R3000A) |
| **Região / ID** | NTSC-U / `SLUS-00923` |
| **Executável Original** | `disc/SLUS_009.23` (Load: `0x80010000`, Entry: `0x80011B80`, Text Size: `0x000D3800`, Stack: `0x801FFFF0`) |
| **Disc Checksums** | MD5: `03ccf2f717dad9168640f518dd4e3e80` \| CRC32: `132bb3e6` |
| **Framework Base** | PSXRecomp (Clean-Room Static Recompiler AOT) + Custom Runtime C/C++ |
| **Repositório Oficial** | `https://github.com/BrazilianHobbyist/Project-RE-Visited-RE3.git` (localizado em `RE3Recomp/`) |

---

## 2. Arquitetura Real do Workspace (Fonte Única de Verdade vs. Distribuição)

```
c:\Users\victo\Documents\Projetos\Decomp - SOTC\
├── WORKFLOW.md                   # Protocolo obrigatório de 6 etapas (Implementar -> Auditar -> Consolidar -> Organizar -> Registrar -> Release)
├── PROGRESS.md                   # Este relatório consolidado de estado e auditoria
├── Jogar RE3 (Launcher).lnk      # Atalho principal -> ResidentEvil3-PCPort\Resident_Evil_3_Launcher.exe
│
├── RE3Recomp/                    # [FONTE ÚNICA DE VERDADE — REPOSITÓRIO GIT]
│   ├── build.ps1                 # Compilação do executável principal (CMake + Ninja)
│   ├── launcher/                 # Código-fonte C++ Win32/GDI+ do Launcher e script build.ps1
│   ├── patches/                  # Patches aplicados no runtime e nos C gerados
│   │   ├── psxrecomp_runtime.patch     # Modificações do runtime consolidado
│   │   └── re3_door_fade_speedup.patch # Redução de frames de fade nas portas (aplicado em generated/)
│   ├── generated/                # 57 shards C gerados das 3.328 funções MIPS de SLUS_009.23
│   ├── seeds/ghidra_funcs.txt    # Mapeamento Ghidra das 3.328 rotinas
│   └── tools/manage_project.ps1  # Script automatizado de Auditoria, Consolidação e Empacotamento Zero-Asset
│
├── ResidentEvil3-PCPort/         # [AMBIENTE DE EXECUÇÃO DO USUÁRIO]
│   ├── Resident_Evil_3_Launcher.exe
│   ├── Resident_Evil_3__Nemesis_Recompiled.exe
│   ├── settings.toml & game.toml # Configurações ativas de vídeo, áudio, controles e cheats
│   ├── bios/                     # openbios.bin (padrão MIT) + SCPH1001.BIN / SCPH5552.BIN
│   ├── gamedata/                 # Imagem .bin/.cue do CD de Resident Evil 3
│   ├── disc/SLUS_009.23          # Executável PS-X EXE extraído do disco
│   └── saves/                    # Memory Cards (.mcd)
│
└── releases/                     # Pacotes .zip limpos (Zero-Asset) para publicação no GitHub Releases
```

---

## 3. Posição Consolidada e Sincera: O Que Funciona vs. O Que Não Está Ativo

### 3.1. Funcionalidades 100% Implementadas, Conectadas e Validadas
1. **Recompilação Estática x86_64 Completa (`Resident_Evil_3__Nemesis_Recompiled.exe`)**:
   - 3.328 funções MIPS compiladas estaticamente em 57 arquivos C (`RE3Recomp/generated/`) + suporte dinâmico a overlays (`0x80165000`, 47 regiões validadas em `overlay_captures.json`).
   - Zero falhas de interpretador e 47/47 overlays válidos.
2. **Launcher Nativo Win32/GDI+ Dark Mode (`Resident_Evil_3_Launcher.exe` v1.5.0)**:
   - Tema escuro ("Raccoon City"), multi-idioma dinâmico (Inglês, Português-BR, Espanhol).
   - **Assistente de Instalação ISO 9660 Mode 2 Form 1**: Extrai `disc/SLUS_009.23` diretamente do `.bin`/`.iso` do usuário e valida a assinatura `PS-X EXE` sem ferramentas externas.
   - **Detecção de Diretório Blindada**: Resolve automaticamente a pasta do jogo (`ResidentEvil3-PCPort` ou raiz) independentemente de onde o Launcher seja iniciado.
   - **Consolidação Definitiva do Fix de DPI / Monitores 4K (`gdiScaling = true`)**: Atendimento direto ao relato do usuário do Reddit. Corrigido o problema de corte inferior de botões no assistente de instalação e janelas do Launcher em telas 4K / High-DPI (125%–250% scaling) através de `AdjustWindowRectEx` em todas as caixas de diálogo e ativação de GDI DPI Scaling no `manifest.xml`, garantindo renderização vetorial nítida e botões 100% visíveis e acessíveis em qualquer monitor moderno.
   - **Layout Limpo e Proporcional**: Reposicionamento harmonioso dos controles após remoção do seletor redundante de FPS, eliminando espaços ociosos.
3. **Door Skip Ágil sem Distorção de Diálogos (`fast_doors = true`)**:
   - Na tabela `0x800979F0`, redireciona o Stage 1 (`0x80015754`, animação 3D da porta) para o Stage 2 (`0x80016150`, finalização) e zera `table[2]`.
   - Fade-out de porta reduzido de 8 para 4 frames (`0x8006FEF0`/`0x8006FF8C`) e fade-in reduzido de 16 para 5 frames (`0x80016190`).
   - Leitura acelerada restrita estritamente a `cdrom_data_read_active()` (`reading && !xa_stream_active`), mantendo `g_turbo_loads_enabled = 0` para que vozes e diálogos rodem sempre na velocidade original.
4. **Skip Ágil de Cutscenes, FMVs e Diálogos Sem Travamentos**:
   - **FMVs (Vídeos e Abertura)**: Pressionar qualquer botão de pulo (`Start`, `Select`, `Ação`, `Esc`, `Espaço`, `Enter`, `Tab`) segura `PAD_START | PAD_SELECT` na porta SIO por 15 frames, acionando o encerramento nativo e limpo do player MDEC do jogo. O leitor de CD-ROM permanece lendo os setores do fluxo normalmente durante o encerramento, eliminando por completo a inanição de buffer que causava o congelamento da imagem em tela.
   - **Cutscenes e Rádios In-Engine**: Pressionar `Select` (botão nativo do RE3 para pular cenas), `Start`, `Esc` ou `Tab` injeta `PAD_SELECT | PAD_CROSS | PAD_START` por 10 frames contínuos no SIO, garantindo que o interpretador de eventos do jogo detecte o comando e avance para o fim da cena.
   - **Entrega Imediata de `CDIRQ_DATA_END` em `cdrom.c`**: Quando um áudio XA é pulado via `cdrom_skip_current_xa()`, o INT4 (`CDIRQ_DATA_END`) é despachado imediatamente (se `irq_flag` for 0 ou `CDIRQ_DATA_READY`) ou no ciclo exato em que a CPU termina de reconhecer o IRQ anterior (`irq_flag == 0`), além de limpar `CDSTAT_READ` no `stat_reg`. Isso elimina a perda de interrupções que fazia a rotina de fala travar em timeout por 3 a 5 segundos antes de pular.
   - **Proteção do Gameplay**: Teclas de ação (`Espaço`, `Enter`, `Cruz`) só avançam diálogos quando há fala XA ou caixa de texto ativa (`is_xa_speaking || is_msg_active`), evitando abertura indesejada de mapa durante exploração e combate.
5. **Controles Modernos 3D no Analógico Esquerdo + D-Pad Tanque (`modern_controls = true`)**:
   - Analógico esquerdo mapeado em 8 setores simétricos com zona morta de `0.26`, caminhada suave e *Auto-Run* ao inclinar acima de `0.82` (exceto ao mirar com `R1`).
   - Girar para Leste/Oeste roda a personagem no próprio eixo sem passos acidentais para frente. O D-Pad permanece 100% com controle tanque clássico.
6. **Vibração / Force Feedback DualShock (`update_controller_rumble`)**:
   - Negociação DualShock forçada ativa (`sio_set_pad_config_capable(..., 1)`) e curva amplificada para motores grave e agudo via SDL2 GameController Rumble (`rumble_intensity` configurável em `settings.toml`).
7. **Gráficos OpenGL HD, Resolução Interna até 4K (SSAA), PGXP Perspectiva e FXAA**:
   - Upscaling interno (`supersampling` 1x a 4x), shader FXAA integrado na apresentação OpenGL, correção de perspectiva de texturas (`perspective_texturing = true`) e proporção 16:9 Esticado ou 4:3 Clássico.
8. **Remoção Completa da Interpolação Artificial de Frames e Opção de Smoothness/FPS (v1.5.0)**:
   - **Decisão e Integridade de Engenharia**: A tentativa de forçar interpolação temporal de frames (60Hz a 240Hz) em um motor 2.5D com fundos pré-renderizados provocava imagem dupla translúcida (*ghosting*), violações do *Painter's Algorithm* de profundidade (Jill desenhada por cima de máscaras 2D do cenário e da janela de inventário) e percepção de placebo.
   - **Ação Executada**: Removida completamente qualquer função de interpolação artificial, blend de quadros, sub-frame drawing de atores e a opção de Smoothness/Refresh Rate do Launcher e das configurações (`settings.toml`, `game.toml`).
   - **Resultado Empírico**: O jogo agora roda com sua renderização e cadência nativas puras de 30 FPS autênticos do motor original do PlayStation 1, com apresentação OpenGL cristalina, resposta de entrada imediata, zero ghosting, zero conflito de camadas 3D/2D e estabilidade absoluta.
9. **Seletor de Cheats Reversível + Atalhos In-Game (`F6`, `F7`, `F8`)**:
   - **Vida Infinita (`0x000CCC90` = 200, `0x000CCC97` = `0x04` Fine)**, **Munição Infinita**, **Fitas Infinitas (`0x81` = 99)**, **Clear Mode Capcom (`0x000D1F32 |= 0xFFE0` para Armas no Baú / Boutique / Mercenaries)** e **8 Epílogos (`0x000D1F3E = 8`)**.
   - Atalhos em tempo real funcionando durante o jogo: `F6` (God Mode), `F7` (Munição Infinita) e `F8` (Munição Máxima no inventário).

---

### 3.2. Funcionalidades Presentes no Código, Mas Desativadas ou Pendentes de Próximo Ciclo (Transparência Total)
1. **Funções QoL de Inventário (`re3_quick_weapon_switch`, `re3_quick_reload`, `re3_combine_ammo_stacks`)**:
   - **Estado Real**: As funções estão escritas em `runtime/src/main.cpp` (linhas 997–1180 do patch) e suas variáveis são lidas no `settings.toml`, **porém não possuem nenhum gatilho de botão/tecla conectado a elas no loop de eventos**. Portanto, hoje são código inativo (*dead code*) até que decidamos atribuir atalhos dedicados ou opções no Launcher.
2. **Pan & Scan Vertical Dinâmico 16:9 (`s_pan_scan_enabled`)**:
   - **Estado Real**: Toda a lógica de rastreamento de centroide vertical 3D da Jill (`note_3d_actor_polygon` em `gpu.c`) e deslocamento suave da câmera com proteção de legendas (`gpu_is_message_box_active` em `gpu_gl_renderer.c`) está compilada no executável. Contudo, na tela do Launcher o seletor de proporção oferece apenas **16:9 Esticado** e **4:3 Clássico** (por estabilidade visual). O Pan & Scan só é ativado se o usuário editar manualmente `pan_scan = true` no `settings.toml`.
3. **Correção Geométrica PGXP (`geometry_correction`)**:
   - **Estado Real**: Funcional no código, mas marcada no Launcher como *"Não recomendado: deforma personagens"* porque os modelos articulados do RE3 sofrem distorção nos membros quando o GTE força precisão sub-pixel nos vértices. A correção de **perspectiva de texturas** (`perspective_texturing`), por outro lado, funciona perfeitamente e vem ativada por padrão.

---

## 4. Comandos Rápidos do Plano de Trabalho (`WORKFLOW.md`)

```powershell
# 1. Auditar estado do Git, paridade SHA-256 dos binários e relatórios de execução:
powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action audit

# 2. Consolidar (atualizar patch do runtime, compilar Launcher, sincronizar binários e docs, limpar dumps):
powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action consolidate

# 3. Gerar pacote de Release Zero-Asset (.zip em releases/):
powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action package -Version "v1.5.0"
```
