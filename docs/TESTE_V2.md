# Roteiro de teste do mantenedor: v2.0.0

Use a pasta de execução (`ResidentEvil3-PCPort`) com o exe e o launcher do commit final. Anote o resultado de cada item: OK, problema (com uma captura) ou não testado. Se algo travar, guarde o `psx_last_run_report.json`.

## 1. Fonte nítida
1. No launcher, marque "Fonte nítida". O texto da opção deve dizer para jogar e reabrir o launcher.
2. Jogue até aparecer um texto (carregar um save já mostra) e feche o jogo.
3. Reabra o launcher: a opção deve dizer "ATIVA".
4. No jogo, confira mensagens, nomes de itens no inventário e o menu GAME CONFIG.

## 2. 16:9 Ampliado
No launcher, escolha a proporção "16:9 Ampliado (sem distorção, menus em 4:3)" e confira:
1. Uma sala com a Jill parada e andando: sem esticar, com a Jill sempre inteira no quadro.
2. Uma troca de câmera: o enquadramento acompanha sem saltos estranhos.
3. Uma mensagem (examinar algo) e a pausa (START): o texto em tamanho normal e o escurecimento na tela toda.
4. Inventário, mapa e arquivos: em 4:3 com barras.
5. Um inimigo na tela (cães ou zumbis): o enquadramento não deve perder a Jill.
6. Uma cutscene com legenda, uma LIVE SELECTION e um FMV.
7. Uma porta (com as portas rápidas ligadas e desligadas).

## 3. Controles modernos (controle do Xbox)
No launcher, escolha os controles modernos.
1. O corredor do save 2 (perto da sala do save, entrada da delegacia): segure uma direção atravessando as trocas de câmera. A Jill não deve inverter nem ir e voltar.
2. Solte o analógico logo depois de uma troca de câmera e empurre de novo: a direção deve seguir a câmera nova.
3. Salas pequenas e escadas.
4. Decida se os controles modernos podem virar padrão.

## 4. Texto de introdução
Um jogo novo até o fim do texto de abertura: a velocidade deve ser a do original.

## 5. Regressões rápidas
- Boot, título, carregar save, salvar na máquina de escrever.
- Portas rápidas, pular FMV, cheats (F6/F7/F8).
- "16:9 esticado" (padrão) e "4:3" continuam como antes.

## Decisões para a release
- Padrões da v2.0.0: 16:9 ampliado e controles modernos ligados ou desligados.
- Publicar (`gh release create`) só depois deste roteiro.
