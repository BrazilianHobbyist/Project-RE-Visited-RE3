# Progresso: Project RE: Visited - RE3

Última revisão: 20/10/2026. Só entra aqui o que foi medido ou verificado. Afirmações sem evidência ficam em "Não verificado".

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

- [ ] v1.7.0: empacotar com o cache gerado a partir do disco (`tools\manage_project.ps1 -Action package` já usa a etiqueta nova).
- [ ] Responder issues (aguardando decisão do mantenedor).
- [ ] Refinar os controles modernos.
- [ ] Decidir o modelo de distribuição: gerar o código recompilado na máquina do usuário (o caminho `build.ps1 -Regen` já é reproduzível) em vez de distribuir o `.exe` com o código do jogo traduzido.
- [ ] Um roteiro curto de teste jogável, com logs, antes de cada release.

## Histórico da auditoria (18–20/10/2026)

- O patch do runtime foi auditado por inteiro.
  - Voltaram ao upstream: `cdrom.c` (o hack de pular falas causava tela preta), o ritmo de quadros e o `config_loader`.
  - Foram removidos: a interpolação morta (placebo), o pan & scan dormente, a câmera do *Muppet Monster* e funções sem gatilho.
- Os créditos foram corrigidos: Ghidra não mapeou as funções (as sementes vêm de uma varredura de `jal`), e o Beetle é só referência do upstream.
- A v1.6.1 saiu com cache de overlays e kit (Python 3.13.1 + TinyCC 0.9.27). Até a v1.6.0, o código das salas rodava interpretado na máquina dos jogadores.
- O launcher foi revisado: recusa ISO de 2048 bytes e discos que não sejam o americano, tem logs honestos nos 3 idiomas e o layout em duas colunas.
- Commits passam a sair como BrazilianHobbyist, há uma única pasta de execução, e as cópias antigas estão em `_arquivo_antigo/`.
