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

## Prioridades

1. Build reproduzível: o `.exe` distribuído deve vir de um commit limpo (o último relatório de execução indica build `-dirty`).
2. Medir e aumentar a cobertura nativa dos overlays; decidir se o compilador vai junto com a release.
3. Decidir o modelo de distribuição: gerar o código recompilado na máquina do usuário em vez de distribuir o `.exe` com o código do jogo traduzido.
4. Um roteiro de teste jogável curto, com logs, antes de cada release.
