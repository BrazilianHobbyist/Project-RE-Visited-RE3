# Relatório de Levantamento Técnico: UI, Launcher & Configuração (R3, R5 & Build Pipelines)

**Data**: 2026-10-18  
**Autor**: Explorer 3 (UI, Launcher & Configuration Explorer)  
**Projeto**: Resident Evil 3: Nemesis Recompiled (RE3Recomp / Project RE: Visited)  
**Escopo**: Requisitos R3 (HD UI & Menus), R5 (Win32 Launcher & Configurações Persistentes) e Pipelines de Compilação (`build.ps1` e `launcher/build.ps1`).

---

## 1. Resumo Executivo

O presente relatório detalha a arquitetura técnica, pontos de acoplamento e plano de implementação para o suporte a texturas e interfaces em alta definição (HD) no porte estático recompilado de *Resident Evil 3: Nemesis*, cobrindo:
1. **Substituição de Telas 2D e Menus (Requisito R3)**: Mecanismos de renderização de telas 2D no PS1 (`POLY_FT4`, `SPRT`, `LoadImage` GP0 0xA0), mapeamento das telas de inventário, status, mapa, galeria de epílogos (1 a 8), máquina de escrever / save e relógio/enigmas, e a estratégia híbrida de substituição (Backdrop HD em `mods/textures/<pack>/ui/` + Fallback/Elementos via hash xxHash64).
2. **Sistema de Configuração (`settings.toml`)**: Análise dos dois subsistemas ativos de parsing no runtime (`config_loader.cpp` via `toml11` e `load_re3_qol_settings()` em `main.cpp`), definição da nova seção `[textures]` e suas estruturas em C/C++.
3. **Arquitetura do Launcher Win32 (`Resident_Evil_3_Launcher.exe`)**: Estrutura do código-fonte em `launcher/main.cpp`, ciclo de vida de `LoadConfig()` / `SaveConfig()`, e o design arquitetural completo para a nova seção/janela "Mods & Texturas HD" com combobox dinâmico de pacotes instalados em `mods/textures/`, checkboxes de controle e atalho direto para o Windows Explorer.
4. **Pipelines de Build**: Diagnóstico detalhado de `build.ps1` (Runtime via CMake + Ninja + Clang/GCC) e `launcher/build.ps1` (Launcher via MinGW g++ + windres), com **validação empírica de compilação limpa (código de saída 0)** em ambos.

---

## 2. Renderização de Telas 2D e Menus no Jogo (Requisito R3)

### 2.1 Como o Hardware PS1 e o RE3 Renderizam Telas 2D

No PlayStation 1, não existe um pipeline separado de 2D e 3D; o chip gráfico (GPU) recebe pacotes de comandos GP0 que escrevem na VRAM de 1024x512 pixels de 16 bits (RGB555 + mask bit). No RE3Recomp, essa VRAM é mantida em alta resolução no framebuffer OpenGL `s_hr_fbo` (`psxrecomp/runtime/src/gpu_gl_renderer.c`, linha 568), com textura `s_hr_tex` escalada por `s_hr_scale`.

As telas 2D do RE3 (inventário, mapa, status, epílogos, save, relógio) utilizam três tipos principais de comandos gráficos:

1. **`POLY_FT4` (GP0 `0x2C` - `0x2F` — Textured Flat Quad)**:
   - Polígonos de 4 vértices texturizados com mapeamento de página (`tpage`) e paleta (`clut`).
   - Como uma página de textura do PS1 tem limite de 256x256 pixels e a resolução do jogo é 320x240, telas cheias de fundo (backgrounds de menus) são montadas usando **múltiplos quads** adjacentes (normalmente 2 quads de 128x240 / 256x240 e 64x240, ou uma grade 2x2).
   - O manipulador central no runtime é `gp0_exec_textured_quad()` em `psxrecomp/runtime/src/gpu.c`.
2. **`SPRT` / `SPRT_16` / `SPRT_8` (GP0 `0x64`, `0x65`, `0x74` - `0x7F` — Textured Rectangles/Sprites)**:
   - Retângulos alinhados aos eixos com dimensões arbitrárias ou fixas (16x16 / 8x8).
   - Utilizados intensivamente para glifos tipográficos (letras/números da fonte), ícones dos itens do inventário (combustível, munição, ervas), cursor de seleção e o gráfico da linha do eletrocardiograma (ECG / Status).
   - Manipulador central: `gp0_exec_textured_rect()` em `psxrecomp/runtime/src/gpu.c` (linha 5444+).
3. **`CPU->VRAM` Copy (GP0 `0xA0` - `0xBF` — `LoadImage`)**:
   - Transferência direta de pixels decodificados da CPU/RAM para a VRAM (`gp0_exec_cpu_to_vram()` em `gpu.c`, linha 5835).
   - É registrada no anel de histórico `a0_history` (`gpu.c`, linha 5800), sendo usada no carregamento de TIMs na memória de vídeo antes dos quads desenharem a tela.

### 2.2 Análise Específica por Tela

| Tela 2D | Estrutura Original (PS1) | Estado em Memória / Gatilho | Nomenclatura Alvo em `mods/textures/<pack>/ui/` |
|---|---|---|---|
| **Inventário (Item Screen)** | Fundo em quads texturizados + grade de 8/10 slots + modelo 3D do item inspecionado + textos | `max_slots` em `0x000D225E` (8 ou 10); slots em `0x000D2134` | `inventory.png` (ou `status.png`) |
| **Condição / Status (ECG)** | ECG animado via sprites/linhas coloridas (Verde/Amarelo/Vermelho) sobreposto ao fundo | Condição do jogador em `0x000CCC97` (0x04=Fine, 0x03=Caution, 0x02=Danger, 0x01=Poison) | `status_fine.png`, `status_caution.png`, `status_danger.png`, `status_poison.png` |
| **Mapa (Map Screen)** | Imagens de planta baixa em quads cobrindo 320x240 + marcadores dinâmicos (seta do jogador, salas visitadas) | Sala ativa em `0x000D1F09`, Stage em `0x000D1F08` | `map.png` (ou `map_stage<stage>.png`) |
| **Galeria de Epílogos** | 8 artes estáticas em tela cheia (320x240) exibidas no final do jogo ou na galeria secreta | Contador de epílogos em `0x000D1F3E` (0 a 8); índice exibido em RAM | `epilogue_0.png` a `epilogue_7.png` (ou `epilogue_1.png` a `epilogue_8.png`) |
| **Máquina de Escrever / Save** | Fundo da sala de save/máquina de escrever + cursor e texto de Memory Card | Rotina de gravação / acesso a memory card | `save.png` (ou `typewriter.png`) |
| **Relógio / Enigmas (Clock Puzzle)** | Fundo estático do mecanismo do relógio da Clock Tower + ponteiros/engrenagens interativas | Sala da Torre do Relógio (`Room` correspondente) | `clock.png` / `puzzle_clock.png` |

### 2.3 Estratégia de Injeção e Hooking no Código

Para atingir fidelidade máxima e compatibilidade perfeita com o projeto *Seamless HD Project (RESHDP)*, a implementação do Requisito R3 deve operar em **dois níveis complementares**:

1. **Nível 1: Substituição de Telas Base (Full-Screen / Backdrop Replacement)**:
   - **Local de Hook**: Em `psxrecomp/runtime/src/gpu.c` (`gp0_exec_textured_quad` e `gp0_exec_textured_rect`) e sincronizado com `gpu_gl_renderer.c`.
   - **Detecção**: Quando um conjunto de quads ou sprite cobrindo as coordenadas da tela inteira (`x0 == 0 && y0 == 0 && (x0+w >= 300) && (y0+h >= 220)`) for submetido e o estado da memória indicar modo de menu (inventário, mapa, save ou epílogo).
   - **Renderização**: O renderizador vincula a textura HD correspondente de `mods/textures/<pack>/ui/<screen>.png` e desenha um quad em alta resolução diretamente no `s_hr_fbo`.
   - **Vantagem**: O fundo é exibido em 1080p/4K cristalino, enquanto os elementos dinâmicos do jogo original (itens no inventário, contadores de munição, legendas e cursores) continuam sendo desenhados nativamente por cima, mantendo 100% de precisão e alinhamento!
2. **Nível 2: Substituição por Fingerprint/Hash de GPU (Compatibilidade RESHDP / Elementos)**:
   - Para ícones individuais, botões, molduras e glifos, cada textura submetida à GPU possui um hash de 64 bits (`xxHash64` sobre tpage, clut e limites UV).
   - O motor busca o arquivo correspondente em `mods/textures/<pack>/ui/<hash>.png` ou `mods/textures/<pack>/replacements/<hash>.png`.
   - Havendo arquivo, substitui o binding da textura pelo asset HD com filtragem linear e UVs normalizadas; não havendo, aplica o fallback original transparente.

---

## 3. Subsistema de Configurações e `settings.toml` (Requisito R5)

### 3.1 Arquitetura Atual de Carregamento no Runtime

O runtime do RE3Recomp possui **dois mecanismos** de ingestão de configurações do `settings.toml`:

1. **Parser do Framework PSXRecomp (`config_loader.cpp`)**:
   - Arquivos: `psxrecomp/recompiler/src/config_loader.h` e `config_loader.cpp`.
   - Biblioteca TOML: **`toml11`** (`toml.hpp`, linha 25 de `config_loader.cpp`), uma biblioteca C++11 header-only robusta e em conformidade estrita com a especificação TOML.
   - Estrutura de Dados: `struct PSXRecompV4::UserSettings` (`config_loader.h`, linhas 1358–1538).
   - Função de Carga: `PSXRecompV4::load_user_settings(const std::filesystem::path& path)` (linha 1552).
   - Escopo: Carrega seções padrão do framework (`[video]`, `[audio]`, `[controller]`, `[bios]`, `[disc]`, `[memcard]`). Se houver erro de sintaxe, renomeia preventivamente o arquivo para `settings.toml.bad` e exibe aviso sem quebrar o executável.
2. **Parser Específico de QoL do RE3 (`load_re3_qol_settings()`)**:
   - Arquivo: `psxrecomp/runtime/src/main.cpp` (linhas 5691–5788).
   - Implementação: Leitura leve e cirúrgica linha por linha (`std::ifstream`), particionando chaves e valores (`key = val`) com trim de aspas e espaços.
   - Chamadas ativas: Invocado em `main.cpp` na inicialização (linha 15926) e no loop de recarga (linha 17452).
   - Variáveis Globais Ativas:
     - `g_fast_doors`: Pulo de animação de portas (0 ou 1).
     - `g_quick_weapon_switch`, `g_quick_reload`, `g_modern_controls`.
     - `g_rumble_intensity`: Intensidade de vibração DualShock (float 0.0 a 5.0).
     - `g_infinite_health`, `g_infinite_ammo`, `g_infinite_ink_ribbons`, `g_all_weapons_box`, `g_unlock_mercenaries`, `g_unlock_boutique`, `g_unlock_epilogues`.
     - `s_aspect_stretch`, `s_pan_scan_enabled`, `s_pan_scan_height`.

### 3.2 Especificação da Nova Seção `[textures]`

Para atender aos requisitos R1, R2, R3, R4 e R5, a seção `[textures]` em `settings.toml` é padronizada da seguinte forma:

```toml
[textures]
# Nome da pasta do pacote de texturas ativo dentro de mods/textures/
# Exemplo: "SeamlessHD", "ClassicHD", ou "" para desativar texturas customizadas
pack            = "SeamlessHD"

# Habilita substituição de cenários pré-renderizados em 4K/HD (R1)
hd_backgrounds  = true

# Habilita substituição de texturas poligonais 3D de modelos e atores (R2)
hd_models       = true

# Habilita substituição de telas 2D de menus, inventário e mapa em HD (R3)
hd_ui           = true

# Habilita ferramenta de despejo (dump) automático de texturas decodificadas em PNG (R4)
dump_textures   = false
```

### 3.3 Estrutura de Dados em C++ para o Runtime

Recomenda-se criar em `psxrecomp/runtime/include/re3_textures.h` (e integrar a leitura em `main.cpp` / `load_re3_qol_settings`):

```cpp
struct RE3TextureConfig {
    std::string pack = "SeamlessHD";
    bool hd_backgrounds = true;
    bool hd_models = true;
    bool hd_ui = true;
    bool dump_textures = false;
};

extern "C" {
    extern struct RE3TextureConfig g_re3_textures;
}
```

No parsing em `load_re3_qol_settings()` (`main.cpp`):
```cpp
else if (key == "pack" || key == "texture_pack") g_re3_textures.pack = val;
else if (key == "hd_backgrounds") g_re3_textures.hd_backgrounds = (val == "true" || val == "1");
else if (key == "hd_models") g_re3_textures.hd_models = (val == "true" || val == "1");
else if (key == "hd_ui") g_re3_textures.hd_ui = (val == "true" || val == "1");
else if (key == "dump_textures") g_re3_textures.dump_textures = (val == "true" || val == "1");
```

---

## 4. Arquitetura do Launcher Win32 (`Resident_Evil_3_Launcher.exe`)

### 4.1 Estrutura do Código-Fonte Atual

O Launcher é um aplicativo Win32 nativo autossuficiente e estático, localizado em `launcher/main.cpp` (2669 linhas, ~142 KB), compilado via MinGW com as seguintes características:
- **Sem frameworks pesados**: Usa pura API Win32 (`CreateWindowW`, `SendMessageW`, `DefWindowProcW`), com subsistema `GDI+` para carregamento de imagens (`banner.jpg` ou recurso RCDATA 2).
- **Tema Visual "Raccoon City"**: Paleta escura consistente (`CLR_BG = RGB(22, 22, 26)`, `CLR_TEXT = RGB(225, 225, 230)`, `CLR_ACCENT = RGB(200, 40, 40)`). Os controles do tipo checkbox/groupbox têm o tema de estilos visuais desativado cirurgicamente via `SetWindowTheme(hChild, L"", L"")` para obedecer à pintura personalizada com `HBRUSH` escuro sem falhas visuais.
- **Multilíngue Dinâmico**: Suporta Inglês (`en`), Espanhol (`es`) e Português Brasileiro (`pt-br`), atualizado em tempo de execução via `ApplyLanguage(g_currentLang)` sem recriar janelas.
- **Janelas Registradas**:
  1. `RE3RecompLauncherClass` (`WndProc`): Janela principal (560 x 808 px).
  2. `RE3CheatsWindowClass` (`CheatsWndProc`): Janela modal de Cheats e Modificadores (580 x 470 px).
  3. `RE3SetupWindowClass` (`SetupWndProc`): Janela de instalação automática / cópia de BIOS e extração de disco (620 x 480 px).
  4. `RE3CreditsWindowClass` (`CreditsWndProc`): Janela de créditos e aviso legal (600 x 520 px).

### 4.2 Ciclo de Leitura e Escrita de Configurações no Launcher

1. **`LoadConfig()`** (linhas 621–687):
   - Localiza `settings.toml` usando `GetSettingsTomlPath()` (que aponta para `<buildDir>\settings.toml`). Se não existir, faz fallback para `GetTomlPath()` (`game.toml`).
   - Lê linha por linha usando `std::wifstream`, ignorando comentários (`#`) e realizando trim de espaços e aspas.
   - Armazena as opções na estrutura global `GameConfig g_cachedConfig`.
2. **`SaveConfig(const GameConfig& cfg)`** (linhas 689–785):
   - Escreve `settings.toml` formatado e estruturado usando `std::wofstream`.
   - Salva blocos determinísticos: `[general]`, `[video]`, `[audio]`, `[controller]`, `[cheats]`.
   - Adicionalmente, atualiza as chaves correspondentes no `game.toml` para consistência.
3. **`GetGuiConfig()`** (linhas 787–835):
   - Coleta o estado ativo dos controles visuais da interface (comboboxes e checkboxes) e preenche a estrutura `GameConfig` antes de gravar no disco.

### 4.3 Design e Implementação da Seção "Mods & Texturas HD"

#### Comparação de Abordagens de Layout:
- **Abordagem A (Janela / Diálogo Dedicado `ShowTexturesDialog`, Padrão Recomendado)**:
  - Criação da classe `RE3TexturesWindowClass` e procedimento `TexturesWndProc`, idêntico ao modelo consolidado do `ShowCheatsDialog` e `ShowCreditsDialog`.
  - Dimensões: 580 x 460 pixels.
  - Acesso: Botão dedicado na janela principal `g_hBtnOpenTextures` ("🎨 Mods & Texturas HD..." / "Mods & HD Textures...") posicionado entre o bloco de Áudio e os botões de ação ou na linha de ferramentas (ao lado de Cheats e Créditos).
  - **Vantagens**:
    1. Espaço generoso para exibir a lista de pacotes instalados, informações da pasta, status dos subcomponentes e explicações claras.
    2. Não expande a altura da janela principal além de 808 px (evitando cortes em telas de notebooks 1366x768 ou 1080p com escala do Windows em 125%/150%).
    3. Isolamento arquitetural limpo no `main.cpp`, facilitando manutenção e testes.
- **Abordagem B (Group Box integrado na janela principal)**:
  - Adicionar `g_hGrpTextures` (`BS_GROUPBOX`) na janela principal.
  - Exige aumentar a altura da janela principal para ~950 px ou reduzir os espaçamentos dos grupos existentes.

#### Especificação dos Novos Controles e IDs:

```cpp
enum ControlID {
    ...
    ID_BTN_OPEN_TEXTURES     = 1038,
    ID_COMBO_TEXTURE_PACKS   = 1039,
    ID_CHK_HD_BACKGROUNDS   = 1040,
    ID_CHK_HD_MODELS         = 1041,
    ID_CHK_HD_UI             = 1042,
    ID_CHK_DUMP_TEXTURES     = 1043,
    ID_BTN_OPEN_MODS_DIR     = 1044,
    ID_BTN_TEXTURES_SAVE     = 1045,
    ID_BTN_TEXTURES_CLOSE    = 1046
};
```

#### Lógica de Enumeração de Pacotes (`mods/textures/`):
```cpp
std::vector<std::wstring> GetInstalledTexturePacks() {
    std::vector<std::wstring> packs;
    packs.push_back(L"None / Vanilla (Desativado)");
    
    std::wstring modsRoot = GetBuildDir() + L"\\mods\\textures";
    CreateDirectoryW((GetBuildDir() + L"\\mods").c_str(), NULL);
    CreateDirectoryW(modsRoot.c_str(), NULL);

    WIN32_FIND_DATAW wfd;
    HANDLE hFind = FindFirstFileW((modsRoot + L"\\*").c_str(), &wfd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                wcscmp(wfd.cFileName, L".") != 0 &&
                wcscmp(wfd.cFileName, L"..") != 0) {
                packs.push_back(wfd.cFileName);
            }
        } while (FindNextFileW(hFind, &wfd));
        FindClose(hFind);
    }
    return packs;
}
```

#### Botão "Abrir Pasta de Mods":
Abre diretamente a pasta no Windows Explorer utilizando a API `ShellExecuteW`:
```cpp
std::wstring modsPath = GetBuildDir() + L"\\mods\\textures";
CreateDirectoryW((GetBuildDir() + L"\\mods").c_str(), NULL);
CreateDirectoryW(modsPath.c_str(), NULL);
ShellExecuteW(NULL, L"open", modsPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
```

#### Atualização de `LoadConfig()` e `SaveConfig()`:
- `GameConfig` ganha:
  ```cpp
  std::wstring texture_pack = L"SeamlessHD";
  bool hd_backgrounds = true;
  bool hd_models = true;
  bool hd_ui = true;
  bool dump_textures = false;
  ```
- `SaveConfig()` grava no arquivo:
  ```cpp
  sFile << L"[textures]\n";
  sFile << L"pack            = \"" << cfg.texture_pack << L"\"\n";
  sFile << L"hd_backgrounds  = " << (cfg.hd_backgrounds ? L"true" : L"false") << L"\n";
  sFile << L"hd_models       = " << (cfg.hd_models ? L"true" : L"false") << L"\n";
  sFile << L"hd_ui           = " << (cfg.hd_ui ? L"true" : L"false") << L"\n";
  sFile << L"dump_textures   = " << (cfg.dump_textures ? L"true" : L"false") << L"\n\n";
  ```

---

## 5. Análise dos Pipelines de Compilação

### 5.1 Pipeline do Launcher (`launcher/build.ps1`)

- **Script**: `launcher/build.ps1` (41 linhas).
- **Ferramentas Invocadas**:
  - `windres.exe` (compilador de recursos Windows): Compila `resource.rc` gerando `resource.o` em formato COFF.
  - `g++.exe` (MinGW GCC): Compila `main.cpp` com `-O2 -static -mwindows -municode`, vinculando estaticamente `-lgdiplus -lcomctl32 -lcomdlg32 -lshlwapi -luxtheme`.
- **Passos Pós-Build**:
  - Copia o executável gerado para `<raiz>\ResidentEvil3-PCPort\Resident_Evil_3_Launcher.exe` (se a pasta existir).
  - Sincroniza `main.cpp` e o executável com `<workspace>\launcher` se invocado a partir do subdiretório `RE3Recomp\launcher`.
- **Resultado do Teste Empírico**:
  - **Status: PASSOU COM 100% DE SUCESSO.**
  - Tempo de execução: ~2,5 segundos.
  - Executável estático de 2,3 MB gerado sem qualquer erro ou aviso.

### 5.2 Pipeline do Runtime Principal (`build.ps1`)

- **Script**: `build.ps1` (127 linhas).
- **Ferramentas Invocadas**:
  - `git`: Verifica e ajusta PATH via `GitPortable` se necessário (`$env:LOCALAPPDATA\Programs\GitPortable\cmd`).
  - `cmake` (v3.20+): Configura o projeto com gerador `Ninja` e tipo `Release` (`-DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`).
  - `ninja`: Compilador multi-thread (`-j <num_cores>`).
  - `python`: Utilizado opcionalmente caso acionada a flag `-Regen` (`recompile.py`).
- **Mecanismo de Patches no Runtime**:
  - Linhas 54–62 verificam e aplicam automaticamente o patch cumulativo `patches/psxrecomp_runtime.patch` no submodule `psxrecomp` via `git apply`.
- **Etapas de Build do CMake**:
  - Compila e vincula `libchdr` (embutido).
  - Vincula `SDL3` do sistema/prebuilt.
  - Vincula backends de BIOS (OpenBIOS e SCPH1001).
  - Compila o código C gerado da decompilação MIPS: `generated/SLUS_009.23_dispatch.c` e todos os shards `generated/SLUS_009.23_full_*.c`.
  - Monta os recursos e ícone do executável (`RE3Recomp_app_icon.rc`).
  - Estágio de catálogo de mods embutidos (5 pacotes de mod do framework em `mods/bundled`).
  - Gera `build/Resident_Evil_3__Nemesis_Recompiled.exe`.
  - Regra de pós-build copia `game.toml` para `build/game.toml`.
- **Resultado do Teste Empírico**:
  - **Status: PASSOU COM 100% DE SUCESSO.**
  - Tempo de execução: ~1,8 segundos (compilação incremental/linking).
  - Executável gerado: `build/Resident_Evil_3__Nemesis_Recompiled.exe`.

---

## 6. Recomendações Técnicas para os Implementadores

1. **Separação Limpa de Responsabilidades**:
   - Manter as definições e rotinas de textura em arquivos modulares dedicados:
     - `psxrecomp/runtime/include/re3_textures.h`
     - `psxrecomp/runtime/src/re3_textures.c` (ou `.cpp`)
   - Isso evita poluir desnecessariamente o arquivo de 19.000 linhas `main.cpp` e facilita a criação de patches limpos.
2. **Prioridade de Resolução de UI (Fallback Transparente)**:
   - Para o Requisito R3, implementar a verificação na seguinte ordem de precedência:
     1. Arquivo de tela cheia nomeado em `mods/textures/<pack>/ui/<screen>.png` (ex: `inventory.png`, `map.png`, `epilogue_<n>.png`).
     2. Arquivo de textura individual por hash em `mods/textures/<pack>/ui/<hash>.png`.
     3. Arquivo de textura genérica em `mods/textures/<pack>/replacements/<hash>.png`.
     4. Renderização original nativa de 32 bits / PS1 (fallback sem falhas).
3. **Padrão de Diálogo Modal no Launcher**:
   - A criação de `ShowTexturesDialog(HWND hWndParent)` em `launcher/main.cpp` oferece a melhor ergonomia de usuário e segurança contra quebra de layout em diferentes resoluções de monitor.
4. **Preservação de Compatibilidade com `game.toml` e `settings.toml`**:
   - Manter a regra do `LoadConfig()`: se `settings.toml` não existir no diretório de execução, inicializar valores a partir de `game.toml` e gravar um `settings.toml` novo na primeira vez que o usuário clicar em "Salvar" ou "Jogar".

---
*Fim do Relatório de Levantamento Técnico.*
