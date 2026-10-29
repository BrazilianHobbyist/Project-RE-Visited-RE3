# Widescreen: o que os dados mostram

Base: 14 capturas automáticas de 21/10/2026 em `captures\ws_auto_20261021` (fora do repositório), feitas na Dark Room da delegacia a partir do save do mantenedor, com 4 quadros cada. Para reproduzir a tabela: `py -3 tools\ws_analyze.py <sessão> --detail <rótulo>`.

## Regra de separação (hipótese de trabalho, consistente em 56/56 quadros)

O RE3 ordena tudo numa tabela de ordenação (OT) por quadro. A posição na OT separa as três classes sem olhar pixels:

| Classe | Assinatura | Casos observados |
|---|---|---|
| **Cena** | primitivas com OT ≥ 16 | exploração: cenário pré-renderizado em Sprites (OT 734–889, CLUT 30720, bloco `0x11xxxx`); Jill em polígonos (OT 722–858, blocos `0x11`/`0x12`); sombra em `PolyFT4+semi` (bloco `0x0C`) |
| **Interface sobre a cena** | OT 0–15 no mesmo quadro que a cena | texto em Sprites na OT 7, CLUT 30736 (branco) e 30864 (destaque verde); escurecimento da pausa em `Rect+semi` na OT 6; tudo no bloco `0x0Dxxxx` (`0x0D4344` retângulo, `0x0D5694…` texto) |
| **Tela 2D inteira** | só OT 0–15, **nenhuma** primitiva de cena | inventário, arquivos (bloco `0x1A`), mapa (`0x1A`/`0x1B`), GAME CONFIG (`0x1C`), menu de carregar (`0x0D` + imagem de fundo por cópia para a VRAM) |

Consequência para o pan & scan:
- **cena** recebe zoom e pan vertical;
- **interface** é desenhada em 1:1, sem corte. O texto fica em y = 185–201 de 240, que é justamente a faixa que um corte de 25% da altura perderia. O escurecimento de tela cheia deve cobrir toda a área 16:9;
- **tela 2D** é apresentada em 4:3 com barras laterais, sem zoom.

A decisão é por quadro e não precisa de detector de menu nem de heurística de texto. Nos dados, a classe de cada quadro foi estável nos 4 quadros seguidos de cada captura.

## Alvo do pan

Na Dark Room, os polígonos da Jill ocupam y ≈ 68–187 de 240. Um corte para 16:9 mostra 180 linhas, então a janela pode começar entre 0 e 60. Centrando na Jill (≈128), a janela começa em ≈38 e ela fica inteira. A posição da Jill no mundo (`0x800CCBC4` +0x34/+0x3C) e a câmera (`0x80098990`) já são lidas pelo controle moderno e gravadas por quadro em `controls.csv`. Isso permite projetar a Jill sem depender dos polígonos.

## Ainda não verificado

- Legendas de cutscene, LIVE SELECTION e cenas com o Nemesis (o roteiro do mantenedor cobre).
- Outras salas: se a faixa OT 0–15 continua exclusiva da interface. Inimigos e efeitos (tiros, sangue) ainda não apareceram nas capturas.
- O texto da tela de carregamento: a captura saiu depois que ele sumiu.
- As coordenadas y alternam entre 0–239 e 240–479 por causa do buffer duplo. A janela do pan precisa ser relativa à área de desenho do quadro.

## Implementação (27/10/2026, `patches/runtime/08-ws-layers.patch`)

Opção "16:9 ampliado" no launcher (`ws_layers = true` no `settings.toml`; desligada por padrão).

- **GPU (`gpu.c`)**: cada comando entra com o rank da OT da lista atual.
  - Quando termina uma lista com primitivas de cena (rank ≥ 16), o renderizador copia a área de desenho, que nesse momento só tem a cena.
  - Uma lista só de interface num quadro sem cena (tela 2D) invalida a cópia daquele buffer.
  - Um retângulo semitransparente que cobre ≥ 90% da tela (escurecimento da pausa) gera uma nova cópia, para o escurecimento ampliar junto com a cena.
  - A caixa vertical dos polígonos opacos da cena (modelos; sombras são semi) e uma assinatura dos sprites de fundo (câmera) vão junto com a cópia.
- **Apresentação (`gpu_gl_renderer.c`)**: um buffer com cópia é composto num FBO 16:9 na escala interna.
  - A cópia ocupa a largura toda, com deslocamento vertical que centra a caixa dos modelos. O pan é suavizado e pula na hora quando a assinatura do fundo muda (troca de câmera).
  - Dentro do retângulo 4:3 central, cada pixel em que a imagem final difere da cópia (a interface) é desenhado 1:1.
  - O resultado passa pelo `present_target_quad` de sempre (redução por área, FXAA).
  - Buffers sem cópia (telas 2D, logos) e FMV ficam em 4:3 com barras.
  - Só o caminho comum de apresentação compõe; com render thread, janela de alta resolução ou blend temporal, fica tudo em 4:3.
- **Debug**: `{"cmd":"ws_layers"}` mostra o estado (cópias, composições, pan, caixas); `on=0|1` liga ao vivo.

**CONFIRMADO** (capturas a 1920×1080 em `captures/ws/`, build de 27/10):
- Dark Room, sala vizinha e troca de câmera: proporção correta, Jill inteira no quadro.
- Mensagem de várias páginas ("An old typewriter." / "You can save your progress here.") em 1:1 sobre a cena.
- Pausa: escurecimento na cena toda ampliada, "Paused" em 1:1.
- Título, menu de load, textos de abertura e inventário: em 4:3 com barras.
- Smoke test OK, a 59,9 FPS, com a opção ligada e desligada. O jogo lê a opção do `settings.toml`, e o launcher grava e relê a escolha.
- Testes do runtime: 221 passam, sem falha nova (stubs novos em `gpu_exec_stubs.inc`).

**NÃO VERIFICADO**:
- Cenas com inimigos (a caixa dos modelos inclui todos eles, então o enquadramento centra o grupo).
- Legendas de cutscene, LIVE SELECTION, mapa e arquivos.
- FMV com a opção ligada.
- Salas cuja altura útil depende do corte de cima (a placa EXIT da Dark Room fica cortada pela metade).

### Correção de 28/10/2026 (relato do mantenedor: "com a opção ativa, nada muda")

**CONFIRMADO**: escolhida pelo launcher, a opção não compunha nenhum quadro.
- **Causa:** `ws_layers` desliga o "esticar", e com isso o RE3 pede apresentação 4:3 (`force_4_3`). O ramo de 4:3 vinha antes do modo em camadas, então o compositor recebia um retângulo 4:3 e recusava.
- **Por que os testes não pegaram:** ligavam as camadas pelo comando de debug, com o esticar ainda ligado. O teste via `settings.toml` só conferia que a opção estava ativa, sem olhar a imagem.
- **Correção:** um buffer com cópia da cena sempre usa o retângulo largo.
- **Diagnóstico novo:** o `ws_layers` agora informa o motivo de cada recusa (`why`: no_copy, invalid, size, scale, narrow) e o último tamanho de tela e de cópia.
- **Teste novo:** `tools/test_settings_e2e.py` dirige o launcher real e confere o efeito de cada escolha no jogo, inclusive se quadros foram compostos. Resultado: 33/33.
