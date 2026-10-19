# Progresso: Project RE: Visited - RE3

Última revisão: 18/10/2026. Só entra aqui o que foi medido ou verificado. Afirmações sem evidência ficam em "não verificado".

## Estado verificado

- **Executável principal:** recompilado estaticamente de verdade (`generated/`, C gerado pelo PSXRecomp a partir de `SLUS_009.23`). Nenhum core de emulador de terceiros embutido.
- **Overlays:** em sua maioria interpretados. Última execução local (com cache): `disp_native=57.891`, `disp_interp=863.131` (~94% interpretado). Em máquinas sem `gcc`/`clang` no PATH não há compilação de overlays, então devem rodar 100% interpretados. O zip de release não inclui compilador nem cache.
- **Hardware (GPU/GTE/SPU/CD/MDEC):** emulado pelo runtime do PSXRecomp.
- **Release v1.5.1:** o SHA-256 do zip bate com o publicado; o `.exe` do jogo no zip = `FB1533D0...`, igual ao local. O launcher local (`E07D45...`) **não** é o publicado (`9E6D29...`).

## Problemas abertos (relatados por usuários)

- Texto de introdução rápido demais.
- Estalos de áudio nas transições de sala.
- PGXP sem efeito / deforma personagens.
- Travamento ao pular a cutscene após entrar por uma porta (fast doors).
- Controles modernos: quebrados na v1.4.x, restaurados na v1.5.1, não retestados.

## Não verificado

Resolução interna, 16:9, FMV skip, vibração, cheats, saves: código presente e opções lidas pelo runtime, mas sem teste reproduzível registrado.

## Pendências da auditoria (18/10/2026)

- [x] Release v1.6.0 publicada (19/10/2026), testada pelo mantenedor.
- Observação: um travamento único no boot da BIOS (`spin_freeze`, frame 152, `0x1FC0CC98`) em 18/10 às 23:19; não se repetiu nas execuções seguintes. Investigar se reaparecer.
- [x] Commits passam a sair como BrazilianHobbyist.
- [x] `build.ps1` aplica `patches/re3_door_*.patch` no código gerado (idempotente, falha visível). Testado com `generated/` limpo e já aplicado.
- [ ] `build.ps1 -Regen` chamava um script inexistente; corrigido para `psxrecomp_cli.py generate`, **não testado**.
- [ ] Auditar o resto de `patches/psxrecomp_runtime.patch` (cheats, skip de FMV/cutscene, vídeo). Áudio já revisado.
- [x] "Controles modernos" reescritos: movimento relativo à tela de verdade (direção da Jill escrita na RAM a partir da câmera), giro com velocidade limitada (`modern_turn_speed`, padrão 90), escadas e empurrar só com "para frente". Testado pelo mantenedor.
- [x] Launcher revisado (v1.6.0):
  - o instalador só aceita o disco USA (`SLUS_009.23`) e recusa ISO de 2048 bytes, com mensagem clara;
  - avisa se o tamanho da imagem difere do dump conhecido;
  - os logs não inventam mais resultados e estão traduzidos nos 3 idiomas;
  - layout em duas colunas (cabe em telas de 768 px);
  - textos de opções, cheats e créditos corrigidos;
  - `modern_turn_speed` é preservado ao salvar, e falhas de gravação são informadas.
  Testado com automação: instalação com o disco real, rejeição de ISO e de disco PAL falsos, capturas em EN e PT-BR.
- [ ] Bloco de "câmera no analógico direito" do *Muppet Monster Adventure* esquecido em `controller_pad_buttons` (sobra de outro projeto; inativo).
- FMV: alguns vídeos só podem ser pulados após certo ponto. Isso é comportamento do jogo original; decidido não contornar.
- [x] Funções de inventário sem gatilho (`re3_quick_weapon_switch`, `re3_quick_reload`, `re3_combine_ammo_stacks`) removidas.
- [x] F8 (munição máxima): ramo do fuzil inalcançável corrigido; a mensagem não promete mais "255 balas".
- Cheats auditados no código: o endereço do inventário (`0x800D2134`) e os IDs de pistola, espingarda, munição e fita conferem com o RAM. **Não verificados:** o byte de status forçado pela vida infinita (`0x800CCC97`), e o fato de "armas no baú", "boutique" e "mercenários" ligarem o mesmo bitmask (`0x800D1F32 |= 0xFFE0`), ou seja, marcar um liga os três. O mantenedor testou os cheats em versões anteriores e eles funcionaram.
- Pulo de cutscene: Esc/Tab no teclado funcionam como Select (pulam cutscenes; em gameplay fazem o que o Select faz). Comportamento aceito.
- [x] Launcher auditado por inteiro (ver item "Launcher revisado (v1.6.0)" acima).
- [x] Cópias antigas da raiz do workspace movidas para `_arquivo_antigo/`; ferramentas não sincronizam mais para lá. Uma única pasta de execução (`ResidentEvil3-PCPort`).
- [x] Release passa a usar `game.toml` e `settings.default.toml` do repositório (a v1.5.1 levou as configurações da pasta de testes, incluindo `frame_interpolation = true`).
- [ ] Responder issues (aguardando decisão do mantenedor).

## Prioridades

1. Build reproduzível: o `.exe` distribuído deve vir de um commit limpo (o último relatório de execução indica build `-dirty`).
2. Medir e aumentar a cobertura nativa dos overlays; decidir se o compilador vai junto com a release.
3. Decidir o modelo de distribuição: gerar o código recompilado na máquina do usuário em vez de distribuir o `.exe` com o código do jogo traduzido.
4. Um roteiro de teste jogável curto, com logs, antes de cada release.
