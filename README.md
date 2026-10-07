# RE3Recomp

**Port nativo de Resident Evil 3: Nemesis (PS1) para Windows**  
Usando o framework [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp).

> ⚠️ **AVISO LEGAL**: Este repositório não contém nenhum dado, asset ou código proprietário da Capcom. Você precisa de uma cópia legal do jogo para gerar o executável.

---

## Features

- 🎮 **Controle Xbox 360** via XInput (plug-and-play)
- 📐 **Resolução 1080p** com suporte a fullscreen (`Alt+Enter`)
- 🖥️ **Widescreen 16:9** — expansão de FOV dos modelos 3D
- 💾 **Saves preservados** — Memory Card virtual em `%APPDATA%\RE3Recomp\saves\`  
  Formato `.mcr` compatível com DuckStation e ePSXe

---

## Pré-requisitos

- Windows 10/11 (64-bit)
- [Visual Studio 2022](https://visualstudio.microsoft.com/) com workload **"Desenvolvimento para Desktop com C++"**
- [Git](https://git-scm.com/)
- [CMake 3.20+](https://cmake.org/)
- [Ninja](https://ninja-build.org/)
- [Python 3.8+](https://www.python.org/)
- [Ghidra](https://ghidra-sre.org/) + Java 17+ (para análise do binário)
- Cópia legal do jogo em formato `.bin`/`.cue`

---

## Estrutura do Projeto

```
RE3Recomp/
├── psxrecomp/              ← Framework PSXRecomp (submodule)
├── seeds/
│   └── functions.csv       ← Lista de funções exportada do Ghidra
├── mods/
│   ├── xinput/
│   │   └── mapping.toml    ← Mapeamento de botões Xbox 360
│   ├── widescreen/
│   │   └── widescreen_patch.c  ← Patch de expansão de FOV 16:9
│   └── saves/
│       └── saves.c         ← Gerenciamento de Memory Card virtual
├── tools/
│   └── ExportRE3Functions.java  ← Script Ghidra para gerar seeds
├── game.toml               ← Configuração central do projeto
├── CMakeLists.txt          ← Build system
├── build.ps1               ← Script de build (PowerShell)
└── README.md
```

---

## Como Usar

### 1. Clonar o Projeto

```powershell
git clone <url-do-seu-repo> RE3Recomp
cd RE3Recomp
git submodule update --init --recursive
```

### 2. Preparar o Disco

```powershell
# Extrair metadados do executável do RE3
.\psxrecomp\tools\new_project_layout\setup_project.ps1 `
  -Disc "C:\caminho\para\RE3.cue" `
  -Dir "."

# Atualizar game.toml com os valores extraídos (load_address, entry_pc, etc.)
```

### 3. Análise no Ghidra (gerar seeds)

1. Abra o Ghidra e crie um novo projeto
2. Importe o executável `SLUS_009.23` como **Raw Binary**, arquitetura **MIPS:LE:32**
3. Base address: `0x80010000` (confirmar com o cabeçalho)
4. Aguarde a auto-análise
5. Vá em **Window > Script Manager**, adicione a pasta `tools/`
6. Execute `ExportRE3Functions.java`
7. Salve o resultado em `seeds/functions.csv`

### 4. Build

```powershell
# Primeira build (com recompilação do binário)
.\build.ps1 -Regen -DiscPath "C:\caminho\para\RE3.cue"

# Builds subsequentes (sem recompilar)
.\build.ps1
```

### 5. Executar

```powershell
.\build\RE3Recomp.exe
```

**Atalhos:**
- `Alt+Enter` — Alternar janela ↔ fullscreen
- `F11` — Fullscreen

---

## Mapeamento de Controle (Xbox 360)

| PS1 (DualShock) | Xbox 360 | Função em RE3 |
|---|---|---|
| × (Cross) | A | Ação / Confirmar |
| ○ (Circle) | B | Cancelar / Usar item |
| □ (Square) | X | Correr |
| △ (Triangle) | Y | Inventário |
| L1 | LB | Mirar |
| R1 | RB | Atirar |
| L2 | LT | Arma anterior |
| R2 | RT | Próxima arma |
| Select | Back | Mapa |
| Start | Start | Pausa |

---

## Saves

Os saves são armazenados em:
```
%APPDATA%\RE3Recomp\saves\MC0.mcr
```

Para importar saves de outro emulador:
```
# Copie o .mcr do DuckStation/ePSXe para:
%APPDATA%\RE3Recomp\saves\MC0.mcr
```

---

## Status

| Feature | Status |
|---|---|
| Boot inicial | 🔧 Em desenvolvimento |
| Gameplay básico | 🔧 Em desenvolvimento |
| Saves | 🔧 Em desenvolvimento |
| XInput | ✅ Configurado |
| Fullscreen | ✅ Configurado |
| Widescreen 16:9 | 🔧 Requer endereços do Ghidra |
| Áudio | 🔧 Em desenvolvimento |

---

## Problemas Conhecidos

- Os fundos pré-renderizados (bitmaps de cenário) permanecem em 4:3 por design do jogo. Apenas os modelos 3D têm FOV expandido.
- RE3 usa dois CDs — suporte ao CD2 pode exigir configuração adicional.

---

## Créditos

- Framework: [PSXRecomp / RetroPortingToolKit](https://github.com/RetroPortingToolKit/psxrecomp)
- Resident Evil 3: Nemesis © Capcom Co., Ltd.
