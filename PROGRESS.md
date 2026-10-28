# Progresso: Project RE: Visited - RE3

Última revisão: 27/10/2026. Só entra aqui o que foi medido ou verificado. Afirmações sem evidência ficam em "Não verificado".

## Estado verificado

- **Executável principal:** recompilado estaticamente de verdade. O C é gerado pelo PSXRecomp a partir do `SLUS_009.23`, em `generated/`, que não é versionado e vem do disco do usuário. Não há core de emulador de terceiros embutido: uma busca no exe por mednafen, libretro, beetle, lightrec, duckstation e pcsx não encontra nada (a única ocorrência de "Beetle" é um comentário de shader).
- **Base:** PSXRecomp `1698a218`, com SDL3. As mudanças do RE3 ficam em `patches/psxrecomp_runtime.patch`.
- **Overlays gerados a partir do disco:** perfil AOT em `aot/`, gerado por `tools/make_aot_profile.py`. Cobre:
  - 17 overlays de menu em `CD_DATA/BIN`;
  - 21 módulos de código de inimigos, membros dos arquivos de sala `R*.BIN`, em dois slots (`0x80100000` / `0x8010D000`). São 42 imagens.

  A auditoria do upstream aprovou 59/59 pares. Toda instrução executada nas 71 capturas de jogo do mantenedor (14.810 de inimigos e 6.266 de menus) está coberta por uma função compilada a partir do disco, com bytes idênticos. Na demonstração da tela de título, só com esse cache e sem compilação em tempo real, nenhuma instrução de inimigo foi interpretada.
- **Reconhecedor de `switch` ampliado:** agora aceita a forma `sltiu; beq; X; lui; addiu; sll`, que aparece em 141 lugares no jogo. A mudança está no C++ e no Python, em paridade e com testes. Etiqueta do cache: `cg18_a8839e97_gcc15c8ed0_f2`.
- **Interpretador:** a medição é por instrução (`interp_hist` no `psx_last_run_report.json`).
  - Cache da v1.6.1: no máximo 0,03%–0,15% em ~20 min.
  - Cache do disco, sessão de 20/10 na delegacia: 0,0061%, com 17 instruções no código de inimigos. O resto é código da BIOS na RAM.
  - O número antigo de "~94% interpretado" vinha de contadores do despachante e não media tempo de execução.
- **Portas rápidas:** o turbo cobre a leitura real da sala, medida pela linha do tempo do CD (`re3_door_timeline`). Primeiro vem o modelo da porta, depois uma pausa e então a leitura da sala. A porta da sala do save caiu de ~3,8 s para ~1 s. Testado pelo mantenedor ("impecáveis").
- **Detector de travamento:** os alarmes falsos (`spin_freeze`) em todo boot eram as telas fixas de aviso e do logo da Capcom. O laço agora precisa ficar parado por 30 s antes de registrar. Num boot de 30 s, nenhum arquivo foi criado.
- **Build reproduzível:** um clone limpo com `build.ps1 -Regen` gera o C idêntico (201/201 arquivos) e o executável e o launcher idênticos byte a byte ao build de desenvolvimento (Clang 22.1.8).
- **Abertura:** a animação da BIOS é pulada (`bios_hle = true`) e o SHA-256 do disco fica memorizado. O DirectInput vem desligado por padrão.

- **PGXP (patches vindos do MMA, 21/10):** dois patches do runtime foram incorporados: rastreamento de vértices guardados com deslocamento de bits e profundidade do canto recortado de quads pelo plano. Medido na Dark Room com a Jill girando, a parcela de texturas sem profundidade (desenhadas sem correção de perspectiva) caiu de 17,3% para 16,1%. Com a geometria ligada só para medir, os triângulos ficaram em 82% precisos, 17% mistos e 0,8% nativos, sem mudança; `cpu_mode` também não muda nada no RE3. Não houve diferença visual. Os testes `pgxp_test`, `texture_plane_depth_test`, `native_wide_projection_test` e `gpu_textured_dot_nw_shift_exec_test` passam; os testes da GPU precisaram de um stub de `fntrace_is_game_started`. Os ~16% restantes têm outra causa, ainda não investigada.

- **Smoke test automático (27/10):** `tools/smoke_test.py` vai do boot ao save "Dark Room" (Memory card 1) e confere pela RAM que a Jill fica controlável, anda com direcional e analógico, abre o inventário e volta a andar depois de fechá-lo. Rodadas de 27/10 com o `build_dbg` de 21/10: todas as checagens OK; 60 FPS no gameplay; 0 underruns; `no_depth` entre 15,2% e 16,1% (varia com o caminho da Jill), na faixa da medição manual de 21/10 (16,1%). O caminho por savestate (`--from-state`) também passou. Um savestate de 20/10 foi recusado pelo build atual, e o teste falhou com o motivo em vez de medir a cena errada.
  - Medidas da rota: o título aparece em ~2.100 quadros, o demo de atração começa em ~2.900 e LOAD GAME já vem selecionado quando há save. A struct da Jill (`0x800CCBC4`) é preenchida durante o texto de abertura do save, mas o byte de modo (+4) só vira 1 quando ela aceita comandos. A flag de UI `0x00080808` (`0x800D4314`) sobe ao abrir o inventário e **não** desce ao fechar.
- **Fonte nítida (27/10, ainda não publicada):** nova opção do launcher. Ela usa o carregador de texturas HD do runtime (`psx.enhancement.hd-textures`, que passa a ir em `mods/bundled`; antes nenhum pacote de `mods/bundled` era copiado e o runtime avisava "not in this build's mod catalog").
  - Na primeira partida com a opção, o carregador extrai os atlas da fonte do jogo do jogador para `mods/hd-pack/sharp-font/dumps`. Ao abrir de novo, o launcher amplia cada atlas 4× com EPX, apaga os dumps que não são da fonte e desliga o dump. Nada derivado do jogo entra no git ou no zip, e o `package` agora grava um `mods/state.toml` limpo.
  - Atlas da fonte: `564BA4ED51F23272` (mensagens, nomes de itens e arquivos; uma imagem por paleta) e `A054F93CA3CFD0A5` (menus GAME CONFIG / KEY CONFIG). Cada paleta é uma substituição separada; glifos fora do retângulo extraído continuam originais.
  - Evidência (capturas a 1920×1080): texto de abertura do save, nome do item no inventário e menu GAME CONFIG ficam com contornos suaves no lugar da escada de pixels (`captures/font_ab_on` × `font_ab_off`, `cfg_on` × `cfg_off`). Os PNGs do launcher (C++) são idênticos pixel a pixel aos do protótipo em Python (`tools/make_font_upscale.py`). `tools/test_launcher_sharp_font.py` dirige o launcher real num sandbox: 16/16 OK. Smoke test com o pacote gerado pelo launcher: todas as checagens OK.
  - Não verificado: arquivos (FILE), mapa e textos com outras paletas, além do limite de tempo das legendas. O interior dos glifos tem um pontilhado que o EPX mantém. O launcher novo ainda não foi compilado na pasta de execução.
- **Ícones de botão Xbox/Nintendo: não aplicável do jeito do MMA (27/10).** No RE3, os botões só aparecem na tela KEY CONFIG (controle, □, R1). Essa tela inteira é um bitmap de 16 bits de 320×240 copiado da CPU para o framebuffer (`copy_cpu_vram`), não uma textura, e o carregador do runtime não substitui escritas de VRAM (`vram-write-*` é recusado pelo parser). A fonte do jogo não tem glifos de botão.

## Problemas abertos

- **Correção geométrica (PGXP):** a solda de vértices fechou as fendas grandes nos rostos, mas ainda sobram pequenas falhas (teste do mantenedor, 20/10). Segue desligada por padrão.
- **Texto de introdução:** avança rápido demais.
- **Controles modernos:** imprecisos em salas pequenas. Desligados por padrão.
- **Overlay de título:** um `switch` do TITLE.BIN usa uma quarta forma ainda não reconhecida, o que dá ~1,9 mil instruções interpretadas por sessão.
- **A reportar ao upstream (decisão do mantenedor):**
  - o resolvedor de tabelas em C++ recusa produtores com fim não alinhado a 4 bytes, e o Python aceita;
  - o `ws_cull_edge_interp_test` não linka;
  - o `test_aot_overlay_discovery.py` depende de um caminho fixo do MinGW;
  - o bug de compilação de overlay no kernel (`func_00003A4C`).

## Não verificado

- Resolução interna, 16:9, vibração e texturas com correção de perspectiva: o código existe e as opções são lidas, mas não há teste reproduzível registrado.
- Cheats: funcionaram em testes antigos do mantenedor e não foram retestados desde a v1.6.0.

## Pendências

- [x] v1.7.0: publicada, com o cache gerado a partir do disco.
- [ ] Widescreen: coletar capturas do warehouse até a delegacia (`docs/WIDESCREEN_PLAYTEST.md`) para separar cena, interface e telas 2D.
- [ ] Responder issues (aguardando decisão do mantenedor).
- [ ] Refinar os controles modernos.
- [ ] Decidir o modelo de distribuição: gerar o código recompilado na máquina do usuário (o caminho `build.ps1 -Regen` já é reproduzível) em vez de distribuir o `.exe` com o código do jogo traduzido.
- [x] Um roteiro curto de teste jogável, com logs, antes de cada release: `tools/smoke_test.py`, no passo 1 da release em `WORKFLOW.md`.

## Histórico da auditoria (18–20/10/2026)

- O patch do runtime foi auditado por inteiro.
  - Voltaram ao upstream: `cdrom.c` (o hack de pular falas causava tela preta), o ritmo de quadros e o `config_loader`.
  - Foram removidos: a interpolação morta (placebo), o pan & scan dormente, a câmera do *Muppet Monster* e funções sem gatilho.
- Os créditos foram corrigidos: Ghidra não mapeou as funções (as sementes vêm de uma varredura de `jal`), e o Beetle é só referência do upstream.
- A v1.6.1 saiu com cache de overlays e kit (Python 3.13.1 + TinyCC 0.9.27). Até a v1.6.0, o código das salas rodava interpretado na máquina dos jogadores.
- O launcher foi revisado: recusa ISO de 2048 bytes e discos que não sejam o americano, tem logs honestos nos 3 idiomas e o layout em duas colunas.
- Commits passam a sair como BrazilianHobbyist, há uma única pasta de execução, e as cópias antigas estão em `_arquivo_antigo/`.
