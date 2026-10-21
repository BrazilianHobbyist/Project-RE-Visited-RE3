# Teste de campo: widescreen (pan & scan em camadas)

Objetivo: coletar dados reais para separar a **cena** (cenário pré-renderizado, modelos 3D, efeitos) da **interface** (legendas, perguntas, seleções) e das **telas 2D inteiras** (inventário, mapa, arquivos). Nada aqui muda o jogo; é só coleta.

## Ferramentas

- **Build de debug:** `build_dbg/` (Release + `PSX_DEBUG_TOOLS=ON`). O build de release (`build/`, o que vai no zip) não tem servidor de debug.
- **Exe de debug na pasta de execução:** `ResidentEvil3-PCPort\Resident_Evil_3__Nemesis_Recompiled_debug.exe`. Usa a mesma pasta, saves, cache e `settings.toml` do jogo normal. Não entra no pacote (o `manage_project.ps1` copia só o exe de release).
- **Servidor TCP:** `127.0.0.1:4370`, uma requisição por conexão.
- **`tools/run_debug.ps1`:** copia o exe de debug, abre o jogo com a mesma linha de comando do launcher e inicia o capturador. `-Build` recompila o `build_dbg` antes, e `-NoCapture` só abre o jogo.
- **`tools/ws_playtest.py`:** o capturador. Fica esperando o **Ctrl+M** do jogo.

## Como rodar

**Não use o launcher.** O launcher abre o exe de release, que não tem servidor de debug: lá o Ctrl+M só mostra "Capture mark needs a debug-tools build" e nada é gravado. Foi o que aconteceu na sessão de 20/10.

1. Feche o jogo, se estiver aberto (a porta é única).
2. Abra o atalho **`Capturas RE3 (Debug)`** na pasta do workspace, ao lado do "Jogar RE3 (Launcher)". Ele abre uma janela do PowerShell com o capturador e o jogo de debug, usando as mesmas configurações que você escolheu no launcher.
   Pelo terminal, o equivalente é `.\tools\run_debug.ps1` dentro de `RE3Recomp`. Se o código do runtime mudou desde o último build de debug, use `.\tools\run_debug.ps1 -Build`; o script avisa quando o build de release é mais novo.
3. Confira que a janela do PowerShell mostra `Connected (...)`. Se mostrar só `Waiting for the game's debug server`, o jogo aberto não é o de debug.
4. Jogue normalmente. Em cada momento do roteiro abaixo, aperte **Ctrl+M** no jogo. Aparece "Capture marked" na tela, e o terminal mostra `[#N] ... capturing` seguido de `-> NNN_frame...`.
5. Para dar nome à captura, digite um rótulo na janela do PowerShell e aperte Enter (ex.: `legenda`). Não é obrigatório, mas ajuda muito.
6. Para encerrar, feche o jogo e aperte Ctrl+C na janela do PowerShell.

Espere a linha `-> NNN_frame...` aparecer antes do próximo Ctrl+M: uma marca enquanto a anterior ainda está sendo salva é ignorada.

## Roteiro sugerido (save do warehouse até a delegacia/Nemesis)

Marque cada situação **com a interface visível na tela**. Quando der, marque também um momento da mesma sala **sem** interface, para comparar.

| # | Situação | Rótulo |
|---|---|---|
| 1 | Exploração normal, Jill andando, sem nada na tela | `explorar` |
| 2 | Legenda de diálogo (ex.: conversa com o Dario no warehouse) | `legenda` |
| 3 | Pergunta ao pegar item ("Will you take it?") com o cursor visível | `pegar-item` |
| 4 | Mensagem de item obtido ou de exame de objeto | `mensagem` |
| 5 | Inventário aberto | `inventario` |
| 6 | Mapa aberto | `mapa` |
| 7 | Arquivos / leitura de documento | `arquivo` |
| 8 | Baú de itens (item box) | `bau` |
| 9 | Máquina de escrever (pergunta de salvar) | `salvar` |
| 10 | LIVE SELECTION com as duas opções na tela | `live-selection` |
| 11 | Cena com o Nemesis (com e sem legenda) | `nemesis` |
| 12 | Sala com câmera aberta, Jill no alto ou no pé da tela | `jill-borda` |
| 13 | Item brilhando perto da borda de cima ou de baixo | `item-borda` |
| 14 | Mira levantada com zumbi na tela | `mira` |

Os itens 12 e 13 servem para medir o risco do corte vertical do pan & scan.

## Controles modernos

O jogo de debug grava, **a cada quadro e o tempo todo**, o que o controle moderno decidiu: posição do stick, caminho do código, ângulo-alvo, proteção de corte de câmera, posição e ângulo da Jill, rotina de animação e câmera. Não precisa apertar nada durante o problema.

1. Deixe `Controles modernos` ligado no launcher (o jogo de debug lê as mesmas configurações).
2. Jogue o trecho problemático normalmente, por exemplo o corredor da sala de save perto da entrada da delegacia.
3. **Até 1 minuto depois** do problema, aperte Ctrl+M. A captura leva o último minuto de controle (`controls.csv`).
4. Rotule descrevendo o que sentiu: `controle: inverteu no corte de camera`, `controle: andou de costas`.

Repetir o mesmo trecho 2 ou 3 vezes, uma marca por tentativa, ajuda a separar o que é sistemático do que é acaso.

## O que cada captura guarda

As sessões ficam em `<workspace>\captures\ws_<data>_<hora>\`, **fora do repositório**. O `index.csv` lista número, pasta, quadro, hora, rótulo e erros. Cada captura tem:

- `frame-00..03.json`: as listas de desenho dos 4 últimos quadros com desenho (o RE3 desenha a 30 fps, então quadros vazios são pulados). Cada primitiva traz a posição na tabela de ordenação (OT), a página de textura, a paleta (CLUT), as UVs e o modo de transparência. O campo `func` aponta para a rotina de DMA da BIOS (`0x00002970`), não para a função do jogo. Quem separa as camadas é a posição na OT.
- `frame-NN.summary.json`: um resumo por função, com opcodes e intervalo de OT.
- `present.png`: o quadro que estava na tela (resolução nativa).
- `gpu_state.json`: o modo de vídeo, a área de desenho e o estado do widescreen.
- `overlay.json`: o estado do carregador de overlays.
- `ram.bin` / `scratchpad.bin`: RAM principal (2 MB) e scratchpad, lidos logo depois da marca. Servem para achar a posição da Jill, a câmera e o overlay carregado. **Contêm dados do jogo: nunca versionar nem publicar.**
- `controls.csv`: o último minuto, uma linha por quadro. Colunas principais:
  - `path`: o que o controle moderno fez. `free` é o movimento relativo à tela; `forward_only` é escada ou empurrar; `exit_*+tank` significa que desistiu e o stick virou controle tanque; `deadzone` é stick solto; `dpad` é o direcional físico.
  - `target_raw` / `target`: o ângulo pedido pelo stick, antes e depois da proteção de corte.
  - `hold` / `cut`: se a proteção de corte estava ativa e se houve corte de câmera naquele quadro.
  - `player_x`, `player_z`, `facing`, `ram_routine`: posição, ângulo e estado de animação da Jill.
  - `cam_x`, `cam_z`, `cam_sig`: posição e assinatura da câmera.

  `frame_history.csv` é o mesmo dado bruto.

## Verificado

- **20/10/2026:** no boot até a demonstração da tela de título, o servidor respondeu e três capturas saíram completas: 4 listas de desenho com cerca de 340 primitivas cada, além de `present.png`, estado da GPU, overlay e RAM.
- **21/10/2026:** o Ctrl+M enviado pelo teclado à janela do jogo gerou uma captura completa, e o atalho abre jogo e capturador juntos. Na demonstração, `controls.csv` mostrou a posição da Jill andando de forma contínua, o ângulo mudando nas curvas, a câmera mudando no corte e a rotina mudando de estado. Com o controle conectado e parado, `path` ficou `deadzone`. O caminho `free` (stick em uso) ainda não foi visto com alguém jogando.
