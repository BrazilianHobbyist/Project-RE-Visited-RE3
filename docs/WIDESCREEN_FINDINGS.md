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
