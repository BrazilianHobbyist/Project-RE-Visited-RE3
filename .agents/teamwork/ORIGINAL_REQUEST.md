# Original User Request

## 2026-10-18T03:22:00Z

Implementar suporte nativo completo a pacotes de texturas e modelos em alta definição (HD) no porte estático recompilado de Resident Evil 3: Nemesis (RE3Recomp / Project RE: Visited), com suporte híbrido ao formato do Seamless HD Project (RESHDP) e substituição de texturas por hash de GPU.

Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp
Integrity mode: demo

## Requirements

### R1. HD Pre-rendered Backgrounds Engine (Fundos 4K / Cenários em Alta Definição)
Implementar mecanismo em OpenGL que detecta o corte de câmera e sala ativos a partir do estado de memória do jogo (`0x000D1F08` Stage, `0x000D1F09` Room, `0x000D1F0A` Cut) e carrega de forma assíncrona/cacheada a imagem de cenário correspondente em alta resolução (1440p/4K) da pasta `mods/textures/<pack>/backgrounds/` (com suporte a nomenclatura padrão RESHDP `R<stage><room>_<cut>.png`). A imagem HD deve ser desenhada na FBO de alta definição (`s_hr_fbo`) com alinhamento pixel-perfect às máscaras de oclusão do motor original.

### R2. 3D Model & Actor HD Texture Replacement (Texturas HD de Modelos 3D e Objetos)
Implementar subsistema de substituição de texturas poligonais 3D (Jill, Nemesis, criaturas, armas e objetos) via fingerprint/hash de 64-bit (`xxHash64` baseado em tpage, clut e limites de textura). Quando uma textura possuir correspondência em `mods/textures/<pack>/replacements/<hash>.png`, o pipeline OpenGL deve vincular a textura RGBA em alta definição, calcular UVs normalizadas e aplicar filtragem anisotrópica (até 16x) e trilinear. Caso não haja substituição, aplicar fallback transparente para a renderização original.

### R3. HD User Interface & Menus (Telas e Menus em Alta Resolução)
Permitir a substituição de telas 2D de interface (inventário, tela de status, mapa, galeria de epílogos e relógio) por versões HD em `mods/textures/<pack>/ui/`.

### R4. Texture Dumping Tooling (Despejo para Criação de Mods)
Disponibilizar rotina de despejo ativável por configuração (`dump_textures = true`), que salva automaticamente em disco todas as texturas de atores e cenários decodificados em formato PNG padronizado com seus respectivos hashes e IDs, permitindo à comunidade criar novos pacotes facilmente.

### R5. Interface no Launcher Win32 e Configurações Persistentes
Adicionar a seção `[textures]` em `settings.toml` e uma aba/seção dedicada "Mods & Texturas HD" no `Resident_Evil_3_Launcher.exe` com seletor de pacotes instalados, checkboxes para Fundos HD, Texturas de Modelos HD e Interface HD, além do atalho para abrir a pasta de mods no Windows Explorer.

## Acceptance Criteria

### Verificação de Carregamento de Cenários HD
- [ ] Cenários em HD/4K carregam sem travamentos ou atrasos perceptíveis durante a troca de ângulos de câmera em gameplay contínuo.
- [ ] Elementos de primeiro plano e máscaras de oclusão mantêm o recorte correto sobre os modelos 3D dos personagens.

### Verificação de Texturas de Modelos 3D
- [ ] Texturas HD para atores 3D são renderizadas sem distorção UV, costuras ou artefatos de borda.
- [ ] A filtragem anisotrópica e trilinear opera ativamente sem afetar a taxa de quadros (30 FPS estável).
- [ ] O mecanismo de fallback para texturas originais do PS1 permanece 100% funcional caso o pacote seja parcial.

### Verificação de Tooling e Launcher
- [ ] O parâmetro `dump_textures = true` grava arquivos PNG válidos e legíveis na pasta de dump especificada.
- [ ] O Launcher lista os pacotes presentes em `mods/textures/` e salva a escolha no `settings.toml`.
- [ ] A compilação completa do runtime (`build.ps1`) e do launcher (`launcher/build.ps1`) conclui sem erros ou avisos críticos.
