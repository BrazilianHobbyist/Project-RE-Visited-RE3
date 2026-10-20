# Progresso: Project RE: Visited - RE3

Última revisão: 19/10/2026. Só entra aqui o que foi medido ou verificado. Afirmações sem evidência ficam em "não verificado".

## Estado verificado

- **Executável principal:** recompilado estaticamente de verdade (`generated/`, C gerado pelo PSXRecomp a partir de `SLUS_009.23`). Nenhum core de emulador de terceiros embutido: busca no exe por mednafen/libretro/beetle/lightrec/duckstation/pcsx não encontra nada (a única ocorrência de "Beetle" é um comentário de shader).
- **Overlays:** recompilados pelo mesmo recompilador em shards nativos (`cache/`). Medição por instrução (histograma `interp_hist` no `psx_last_run_report.json`, adicionado em 19/10): em ~20 min de jogo em 4 sessões, instruções interpretadas ≤ 0,03%–0,15% dos ciclos da CPU emulada.
  - **Correção:** o número antigo "~94% interpretado" vinha de `disp_native`/`disp_interp`, que contam entradas no despachante (uma função nativa inteira conta 1; o interpretador conta por bloco). Não mede tempo de execução e superestima muito o interpretador.
  - Até a v1.6.0 o zip não levava cache nem `overlay_toolchain`, então na máquina do jogador as salas rodavam sempre interpretadas. A v1.6.1 inclui os dois (567 shards; Python 3.13.1 + TinyCC 0.9.27). Testado numa pasta limpa sem gcc: o cache é usado, e o kit compila sozinho com TinyCC (144 shards, 0 falhas).
- **Cache de overlays:** gerado com `compile_overlays.py --cps` a partir dos snapshots em `ResidentEvil3-PCPort/overlay_captures.json.d` (capturas de jogo do mantenedor; privadas, nunca no pacote). Tag `cg18_d1867bb4_gcc15c8ed0_f0`, igual à do `game.toml` da release.
- **Hardware (GPU/GTE/SPU/CD/MDEC):** emulado pelo runtime do PSXRecomp.
- **Abertura:** animação da BIOS pulada (`bios_hle = true`); SHA-256 do disco memorizado em `%LOCALAPPDATA%\psxrecomp\disc_sha256_memo.txt`; DirectInput desligado por padrão (o `SDL_DINPUT_JoystickDetect` travava ~9 s no PC do mantenedor). Janela em 0,4 s a partir da segunda abertura.

## Problemas abertos

- Texto de introdução rápido demais.
- PGXP deforma personagens.
- Controles modernos: imprecisos em salas pequenas ("desgovernados"); desligados por padrão até o refino.
- `spin_freeze` registrado por volta do quadro 130–170 em toda abertura. O jogo segue normalmente (quadros a 60 fps); parece alarme falso do detector durante a espera da BIOS. Não confirmado.

## Não verificado

Resolução interna, 16:9, vibração e texturas com correção de perspectiva: código presente e opções lidas pelo runtime, mas sem teste reproduzível registrado. Cheats funcionaram em testes antigos do mantenedor; não retestados na v1.6.1.

## Pendências da auditoria (18/10/2026)

- [x] Release v1.6.0 publicada (19/10/2026), testada pelo mantenedor.
- Observação: um travamento único no boot da BIOS (`spin_freeze`, frame 152, `0x1FC0CC98`) em 18/10 às 23:19; não se repetiu nas execuções seguintes. Investigar se reaparecer.
- [x] Commits passam a sair como BrazilianHobbyist.
- [x] `build.ps1` aplica `patches/re3_door_*.patch` no código gerado (idempotente, falha visível). Testado com `generated/` limpo e já aplicado.
- [ ] `build.ps1 -Regen` chamava um script inexistente; corrigido para `psxrecomp_cli.py generate`, **não testado**.
- [x] Patch do runtime auditado por inteiro (19/10/2026). Revertidos ao upstream: `cdrom.c` (o hack de pular falas tinha mudado o modelo do drive para todas as leituras), ritmo de quadros (o relógio só rodava no netplay), `config_loader`. Removidos: interpolação de geometria morta (gerava `interp_audit.log`), pan & scan dormente (rastreava todo polígono), opção de câmera do *Muppet Monster*, variáveis mortas. Mantidos: FXAA, esticar 16:9 / 4:3 com bordas, recorte dos FMVs, controles, portas, áudio, cheats, vibração.
- [x] "Controles modernos" reescritos: movimento relativo à tela de verdade (direção da Jill escrita na RAM a partir da câmera), giro com velocidade limitada (`modern_turn_speed`, padrão 90), escadas e empurrar só com "para frente". Testado pelo mantenedor.
- [x] Launcher revisado (v1.6.0):
  - o instalador só aceita o disco USA (`SLUS_009.23`) e recusa ISO de 2048 bytes, com mensagem clara;
  - avisa se o tamanho da imagem difere do dump conhecido;
  - os logs não inventam mais resultados e estão traduzidos nos 3 idiomas;
  - layout em duas colunas (cabe em telas de 768 px);
  - textos de opções, cheats e créditos corrigidos;
  - `modern_turn_speed` é preservado ao salvar, e falhas de gravação são informadas.
  Testado com automação: instalação com o disco real, rejeição de ISO e de disco PAL falsos, capturas em EN e PT-BR.
- [x] Bloco de "câmera no analógico direito" do *Muppet Monster Adventure* removido (runtime e config_loader).
- FMV: alguns vídeos só podem ser pulados após certo ponto. Isso é comportamento do jogo original; decidido não contornar.
- [x] Funções de inventário sem gatilho (`re3_quick_weapon_switch`, `re3_quick_reload`, `re3_combine_ammo_stacks`) removidas.
- [x] F8 (munição máxima): ramo do fuzil inalcançável corrigido; a mensagem não promete mais "255 balas".
- Cheats auditados no código: o endereço do inventário (`0x800D2134`) e os IDs de pistola, espingarda, munição e fita conferem com o RAM. **Não verificados:** o byte de status forçado pela vida infinita (`0x800CCC97`), e o fato de "armas no baú", "boutique" e "mercenários" ligarem o mesmo bitmask (`0x800D1F32 |= 0xFFE0`), ou seja, marcar um liga os três. O mantenedor testou os cheats em versões anteriores e eles funcionaram.
- Pulo de cutscene: Esc/Tab no teclado funcionam como Select (pulam cutscenes; em gameplay fazem o que o Select faz). Comportamento aceito.
- [x] Launcher auditado por inteiro (ver item "Launcher revisado (v1.6.0)" acima).
- [x] Cópias antigas da raiz do workspace movidas para `_arquivo_antigo/`; ferramentas não sincronizam mais para lá. Uma única pasta de execução (`ResidentEvil3-PCPort`).
- [x] Release passa a usar `game.toml` e `settings.default.toml` do repositório (a v1.5.1 levou as configurações da pasta de testes, incluindo `frame_interpolation = true`).
- [ ] Responder issues (aguardando decisão do mantenedor).

- [x] v1.6.1 preparada (19/10/2026): cache de overlays + kit no pacote, correção da tela preta ao pular falas (hack `cdrom_skip_current_xa` removido), abertura rápida, controle clássico como padrão.

## Prioridades

1. Build reproduzível: o `.exe` distribuído deve vir de um commit limpo (o último relatório de execução indica build `-dirty`).
2. Ampliar o cache de overlays jogando mais áreas; avaliar a extração AOT a partir do disco (`docs/AOT_SHARDING.md` do PSXRecomp) para não depender de capturas.
3. Refinar os controles modernos (salas pequenas).
4. Decidir o modelo de distribuição: gerar o código recompilado na máquina do usuário em vez de distribuir o `.exe` com o código do jogo traduzido.
5. Um roteiro de teste jogável curto, com logs, antes de cada release.
