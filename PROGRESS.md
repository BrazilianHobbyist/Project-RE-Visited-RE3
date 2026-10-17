# Project RE: Visited — Resident Evil 3: Nemesis
## Relatório Consolidado de Auditoria, Memória e Progresso do Projeto

> **Data da Última Auditoria**: 17 de Outubro de 2026  
> **Status Geral**: Auditado, sincronizado e consolidado (v1.4.3+ / Build de 17/10/2026). As 3 cópias do executável do jogo e as 3 cópias do Launcher estão 100% idênticas (verificadas via SHA-256). Plano de governança persistente ativo em [`WORKFLOW.md`](WORKFLOW.md) e automatizado por [`tools/manage_project.ps1`](RE3Recomp/tools/manage_project.ps1).

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
│   │   ├── psxrecomp_runtime.patch     # Modificações do runtime (88 KB)
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
   - Última sessão registrada em `psx_last_run_report.json`: **7.969 frames** executados com encerramento limpo (`reason: atexit`, `exit_origin: sdl_window_close`), zero falhas de interpretador e 47/47 overlays válidos.
2. **Launcher Nativo Win32/GDI+ Dark Mode (`Resident_Evil_3_Launcher.exe`)**:
   - Tema escuro ("Raccoon City"), multi-idioma dinâmico (Inglês, Português-BR, Espanhol).
   - **Assistente de Instalação ISO 9660 Mode 2 Form 1**: Extrai `disc/SLUS_009.23` diretamente do `.bin`/`.iso` do usuário e valida a assinatura `PS-X EXE` sem ferramentas externas.
   - **Detecção de Diretório Blindada**: Resolve automaticamente a pasta do jogo (`ResidentEvil3-PCPort` ou raiz) independentemente de onde o Launcher seja iniciado.
3. **Door Skip Ágil sem Distorção de Diálogos (`fast_doors = true`)**:
   - Na tabela `0x800979F0`, redireciona o Stage 1 (`0x80015754`, animação 3D da porta) para o Stage 2 (`0x80016150`, finalização) e zera `table[2]`.
   - Fade-out de porta reduzido de 8 para 4 frames (`0x8006FEF0`/`0x8006FF8C`) e fade-in reduzido de 16 para 5 frames (`0x80016190`).
   - Leitura acelerada restrita estritamente a `cdrom_data_read_active()` (`reading && !xa_stream_active`), mantendo `g_turbo_loads_enabled = 0` para que vozes e diálogos rodem sempre na velocidade original.
4. **Skip Nativo de Diálogos e Cutscenes XA (Novo — Build de 17/10/2026)**:
   - Implementado `cdrom_skip_current_xa()` em `cdrom.c` e conectado ao loop por frame em `main.cpp`.
   - Pressionar Ação (`X`/Cruz, Espaço, Enter) ou `Start`/`Select`/`Esc`/`Tab` durante falas/monólogos em streaming XA encerra o stream XA imediatamente e aciona o handler nativo de fim de fala do jogo, sem acelerar o clock nem distorcer o áudio.
5. **Controles Modernos 3D no Analógico Esquerdo + D-Pad Tanque (`modern_controls = true`)**:
   - Analógico esquerdo mapeado em 8 setores simétricos com zona morta de `0.26`, caminhada suave e *Auto-Run* ao inclinar acima de `0.82` (exceto ao mirar com `R1`).
   - Girar para Leste/Oeste roda a personagem no próprio eixo sem passos acidentais para frente. O D-Pad permanece 100% com controle tanque clássico.
6. **Vibração / Force Feedback DualShock (`update_controller_rumble`)**:
   - Negociação DualShock forçada ativa (`sio_set_pad_config_capable(..., 1)`) e curva amplificada para motores grave e agudo via SDL2 GameController Rumble (`rumble_intensity` configurável em `settings.toml`).
7. **Gráficos OpenGL, Resolução Interna até 4K (SSAA), PGXP Perspectiva, FXAA e Cadência 60–240 Hz (`frame_interpolation`)**:
   - Upscaling interno (`supersampling` 1x a 4x), shader FXAA integrado na apresentação OpenGL, correção de perspectiva de texturas (`perspective_texturing = true`) e proporção 16:9 Esticado ou 4:3 Clássico.
   - **Sub-Frame Pacer OpenGL 60–240 Hz + Interpolação Temporal Motion-Adaptive (Corrigido — Build de 17/10/2026)**:
     - Corrigidos 3 bugs críticos deixados em `main.cpp` pela sessão anterior: (1) removido o override hardcoded `PSX_MOD_FRAME_INTERPOLATION_HOLD` que travava `alpha = 1.0f` e impedia qualquer interpolação sem Render Pass; (2) restaurado `if (g_frame_interpolation) return 0;` em `present_effective_swap_interval()` para que o `SDL_GL_SwapWindow` não bloqueie o *pacer* de alta frequência (`interp_present_source_interval`); e (3) reaplicado `load_re3_qol_settings()` após `start_mod_session()` para impedir que `reset_mod_owned_presentation()` sobrescreva as configurações do Launcher.
8. **Seletor de Cheats Reversível + Atalhos In-Game (`F6`, `F7`, `F8`)**:
   - **Vida Infinita (`0x000CCC90` = 200, `0x000CCC97` = `0x04` Fine)**, **Munição Infinita**, **Fitas Infinitas (`0x81` = 99)**, **Clear Mode Capcom (`0x000D1F32 |= 0xFFE0` para Armas no Baú / Boutique / Mercenaries)** e **8 Epílogos (`0x000D1F3E = 8`)**.
   - Atalhos em tempo real funcionando durante o jogo: `F6` (God Mode), `F7` (Munição Infinita) e `F8` (Munição Máxima no inventário).

---

### 3.2. Funcionalidades Presentes no Código, Mas Desativadas ou Pendentes de Próximo Ciclo (Transparência Total)
1. **Interpolação Geométrica / Esquelética 3D Real via Render Pass (Planejada para o Próximo Ciclo — Etapa 2)**:
   - **Estado Real**: Atualmente a fluidez de 60–240 Hz opera via **apresentação temporal Motion-Adaptive no OpenGL** (subdividindo os quadros de 30 FPS do jogo na taxa nativa do monitor sem *ghosting* em cortes de câmera). A interpolação poligonal 3D das entidades (`0x800CCC00`, recalculando matrizes GTE e re-submetendo a *Ordering Table* entre `0x80023F38` e `0x80078620`) foi removida na sessão anterior após um conflito com um hook de *tasks* e será reimplementada de forma limpa e isolada no próximo ciclo do `WORKFLOW.md`.
2. **Funções QoL de Inventário (`re3_quick_weapon_switch`, `re3_quick_reload`, `re3_combine_ammo_stacks`)**:
   - **Estado Real**: As funções estão escritas em `runtime/src/main.cpp` (linhas 997–1180 do patch) e suas variáveis são lidas no `settings.toml`, **porém não possuem nenhum gatilho de botão/tecla conectado a elas no loop de eventos**. Portanto, hoje são código inativo (*dead code*) até que decidamos atribuir atalhos dedicados ou opções no Launcher.
3. **Pan & Scan Vertical Dinâmico 16:9 (`s_pan_scan_enabled`)**:
   - **Estado Real**: Toda a lógica de rastreamento de centroide vertical 3D da Jill (`note_3d_actor_polygon` em `gpu.c`) e deslocamento suave da câmera com proteção de legendas (`gpu_is_message_box_active` em `gpu_gl_renderer.c`) está compilada no executável. Contudo, na tela do Launcher o seletor de proporção oferece apenas **16:9 Esticado** e **4:3 Clássico** (por estabilidade visual). O Pan & Scan só é ativado se o usuário editar manualmente `pan_scan = true` no `settings.toml`.
4. **Correção Geométrica PGXP (`geometry_correction`)**:
   - **Estado Real**: Funcional no código, mas marcada no Launcher como *"Não recomendado: deforma personagens"* porque os modelos articulados do RE3 sofrem distorção nos membros quando o GTE força precisão sub-pixel nos vértices. A correção de **perspectiva de texturas** (`perspective_texturing`), por outro lado, funciona perfeitamente e vem ativada por padrão.
5. **Diagnóstico dos Dumps Antigos de `spin_freeze` na BIOS**:
   - **Estado Real**: Os 4 arquivos `psx_freeze_dump_*.json` encontrados no workspace ocorreram entre os frames 158 e 274 no endereço `0x1FC0B...` (ROM da BIOS) quando o `bios.cfg` antigo de `RE3Recomp` apontava para a BIOS retail europeia (`SCPH5552.BIN`). Com a padronização para `bios/openbios.bin` em todos os arquivos `bios.cfg`, o boot ocorre sem travamento. Os dumps antigos foram auditados e limpos.

---

## 4. Comandos Rápidos do Plano de Trabalho (`WORKFLOW.md`)

```powershell
# 1. Auditar estado do Git, paridade SHA-256 dos binários e relatórios de execução:
powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action audit

# 2. Consolidar (atualizar patch do runtime, compilar Launcher, sincronizar binários e docs, limpar dumps):
powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action consolidate

# 3. Gerar pacote de Release Zero-Asset (.zip em releases/):
powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action package -Version "v1.4.4"
```
