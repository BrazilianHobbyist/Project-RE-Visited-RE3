# Project RE: Visited — Resident Evil 3: Nemesis
## Relatório de Estado, Memória e Progresso do Projeto

> **Data de Atualização**: 15 de Outubro de 2026  
> **Status Geral**: Executável funcional, auditado e estabilizado (v1.4.3 — `Resident_Evil_3__Nemesis_Recompiled.exe`), timing e velocidade autêntica de diálogos preservados, transições de porta e cortes de câmera suaves a 60 FPS estáveis.

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

---

## 2. Arquitetura do Projeto

```
c:\Users\victo\Documents\Projetos\Decomp - SOTC\
├── game.toml                     # Configuração central de carregamento, endereços e runtime
├── settings.toml                 # Preferências de vídeo, áudio, controles e resolução
├── Resident_Evil_3__Nemesis_Recompiled.exe  # Executável do jogo compilado (45 MB)
├── Jogar RE3 (Launcher).lnk      # Atalho direto para o launcher
│
├── RE3Recomp/                    # Código-fonte e pipeline de compilação
│   ├── build.ps1                 # Script mestre de compilação (CMake + Ninja)
│   ├── patches/                  # Patches aplicados no runtime do PSXRecomp
│   │   └── psxrecomp_runtime.patch  # 80 KB de modificações (Door Skip, Cheats, Vibração, PGXP, UI)
│   ├── seeds/                    # Mapeamento de funções Ghidra (3.328 rotinas MIPS)
│   │   └── ghidra_funcs.txt
│   └── CMakeLists.txt
│
├── ResidentEvil3-PCPort/          # Pasta de distribuição/execução com assets, configs e saves
│   ├── Resident_Evil_3_Launcher.exe
│   ├── input.ini & keybinds.ini
│   ├── overlay_captures.json     # Rastreamento de overlays de memória
│   └── psx_last_run_report.json  # Relatório do último encerramento/diagnóstico de freeze
│
└── gamedata/ / disc/             # Dados originais extraídos do disco
```

---

## 3. Mapeamento de Memória & Endereços Críticos (Engenharia Reversa)

Estes são os endereços estáticos descobertos e utilizados no código dos patches:

### Transições de Cenário & Portas (Door Skip Ultra-Rápido com Fade Suave)
- **`0x800979F0`** — **Tabela de Estágios de Transição de Sala**:
  - `table[0]` (`0x80015498`): **Stage 0 (Carregamento de Dados)**: Leitura de dados do cenário e modelos do CD-ROM.
  - `table[1]` (`0x80015754`): **Stage 1 (Animação 3D de Porta e Áudio)**: No Door Skip, redirecionado diretamente para `0x80016150`.
  - `table[2]` (`0x80016150`): **Stage 2 (Finalização e Fade-in)**: No Door Skip, zerado (`0x00000000`) para evitar execução duplicada de Stage 2.
- **Aceleração de Carregamento Estrita (`cdrom_data_read_active`)**:
  - A aceleração unpaced do Door Skip atua **exclusivamente** durante a leitura física de setores do CD (`cdrom_data_read_active() = reading && !xa_stream_active`), terminando no instante exato em que os setores são entregues.
  - O sistema genérico de `turbo_loads` do PSXRecomp é mantido explicitamente desativado (`g_turbo_loads_enabled = 0`), impedindo que a janela de tolerância de 30 frames (`CD_BURST_GAP_FRAMES`) e os frames de liberação acelerem indevidamente os diálogos da Jill ao carregar saves ou arremessem a personagem bruscamente nas transições de cena/ângulo.
  - Fade-in de 5 frames restaurado a 60 FPS estáveis, garantindo apresentação visual suave e polida.

### Desbloqueios Nativos da Capcom (Clear Mode / Segredos)
- **`0x800D1F32`** — **Clear Mode / Secret Flag**:
  - Bitmask `0xFFE0`: Ativa o modo limpo nativo da Capcom, liberando a chave da Boutique, todas as roupas extras e abastecendo o baú com armas especiais infinitas (Gatling Gun, Rocket Launcher, M4A1 com munição ilimitada).
- **`0x000D1F3E`** — **Epilogue Gallery Counter**:
  - Definido como `8`: desbloqueia todos os 8 epílogos originais na galeria do menu principal.

### Estado do Jogador & Cheats
- **`0x000CCC97`** — **Health Condition da Jill**:
  - Byte de status: `0x04` = Condição *Fine* e remoção imediata de veneno (*Poison cleared*). Usado no patch de God Mode.

### IDs de Munição Mapeados
- Pistola 9mm: `0x15` | Pistola Reforçada: `0x24`
- Shotgun: `0x17` | Shotgun Reforçada: `0x25`
- Magnum: `0x16`
- Lança-Granadas Normal: `0x18` | Flame: `0x19` | Acid: `0x1A` | Freeze: `0x1B`
- Mine Thrower: `0x1F`
- Fuzil de Assalto M4A1: `0x22`

---

## 4. Funcionalidades Já Implementadas e Validadas

1. **Recompilação Estática x86_64 Completa**:
   - 3.328 rotinas MIPS R3000A compiladas em código nativo de máquina via `build.ps1`.
2. **Door Skip Opcional Instantâneo**:
   - Bypass automático do carregamento falso de 3 segundos entre portas.
3. **PGXP (Precision Geometry Transform Pipeline)**:
   - Elimina o efeito "gelatina" (wobbling de texturas e vértices tremendo) característico do hardware original do PS1.
   - Texturização com correção de perspectiva e renderização em OpenGL.
4. **Vibração Háptica Realista**:
   - Ajuste dinâmico de curvas para controle DualShock / XInput: motor pesado com sustentação para tiros de grosso calibre e motor leve de alta frequência para impactos e tiros rápidos.
5. **Suporte a Widescreen & Resoluções Modernas**:
   - Seleção de proporção 16:9 ou 4:3 clássica em tela cheia e modo janela.
6. **Painel de Configuração & Launcher**:
   - Executável launcher para mapear botões, alternar filtros visuais e gerenciar saves.
7. **Sistema de Telemetria de Freeze**:
   - Geração automática de `psx_freeze_dump_*.json` e `psx_last_run_report.json` para capturar registradores, ponteiros de pilha e scratchpad caso ocorra desvio de fluxo.

---

## 5. Como Recompilar e Rodar

Para compilar qualquer alteração futura no código ou nos patches:

```powershell
# Dentro da pasta RE3Recomp:
.\build.ps1 -Config Release
```

Para executar o jogo:
- Diretamente pelo executável: `.\Resident_Evil_3__Nemesis_Recompiled.exe`
- Pelo launcher com menu: `.\ResidentEvil3-PCPort\Resident_Evil_3_Launcher.exe`

---

## 6. Backlog para o Retorno

Quando voltarmos a mexer neste projeto, os pontos prioritários em aberto são:
1. **Monitoramento de Overlays do Modo Mercenaries**: Verificar se todas as salas específicas da missão de evacuação carregam perfeitamente sem congelamento na troca de overlay (`0x80165000`).
2. **Ajustes de Áudio Específicos**: Validar o comportamento de reverberação de passos e eco de tiros nas salas de subsolo.
3. **Empacotamento de Release**: Criar instalador limpo para distribuição se desejado.
