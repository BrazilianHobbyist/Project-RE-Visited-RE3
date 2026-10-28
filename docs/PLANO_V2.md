# Plano de ação até a v2.0.0

Escrito em 27/10/2026. Nenhuma release sai sem aprovação explícita do mantenedor. Cada fase termina em commit no `main` (push liberado), com evidência citada no `PROGRESS.md`. Status permitidos: CONFIRMADO, HIPÓTESE, REFUTADO.

## Fase 0: fechar o que estava aberto (feita)

- Commit `1c58ffb` (push feito): smoke test, opção "Fonte nítida", pacote hd-textures em `mods/bundled` e `state.toml` limpo no zip.
- Pendente do mantenedor: testar a fonte nítida no jogo real.

## Fase 1: manutenção do runtime

1. Dividir `patches/psxrecomp_runtime.patch` (140 KB) em patches por tema (`patches/runtime/NN-<tema>.patch`). Cada um deve aplicar em ordem sobre o upstream, e o `build.ps1` aplica todos.
   - Aceite: o build fica byte a byte igual ao de antes da divisão.
2. Aplicar `pgxp-texture-zguard.patch` (MMA, 23/10).
   - Aceite: `no_depth` medido pelo smoke test antes e depois, e capturas sem leques de textura no pé da tela.
3. Atualizar o PSXRecomp de `1698a218` para `958ae0e3`, que traz a correção de VSync em monitores de 120/144 Hz.
   - Reaplicar os patches e resolver conflitos.
   - Se a etiqueta do recompilador mudar, regenerar o C e o cache de overlays (auditoria `all_pairs_valid`).
   - Aceite: smoke test OK, testes do runtime sem falhas novas em relação ao upstream puro e reprodutibilidade (`-Regen`) mantida.

## Fase 2: PGXP de geometria (pergunta 5)

Hoje ele fica desligado: a solda de vértices fechou as fendas grandes, mas sobram pequenas falhas nos rostos (teste do mantenedor, 20/10).

1. Medir com o smoke test, com a geometria ligada pelo debug server: `tri_precise` / `mixed` / `native` e capturas de rosto em close (Jill na Dark Room, sala do save).
2. Localizar as falhas restantes com o anel de triângulos (`pgxp_tri_ring`, campo `tex`) e as ferramentas do MMA (`tex_seams`, `pgxp_cracks`).
3. Corrigir só o que for medido. Máximo de 3 abordagens; depois disso, registrar e seguir.
4. Decisão para a v2.0.0:
   - ligar por padrão só se as capturas de rosto, cenário e inimigos não tiverem regressão;
   - se não, fica como opção "experimental" no launcher, desligada.

**Resposta honesta à pergunta 5:** não dá para prometer o PGXP de geometria ligado por padrão na v2.0.0 antes da etapa 1. O critério de decisão está acima.

## Fase 3: widescreen em camadas (autônomo)

O que já se sabe (`docs/WIDESCREEN_FINDINGS.md`): ranks 0–15 da ordem de desenho são interface, 16 ou mais são cena, e só a camada de fundo indica uma tela 2D cheia. Isso valeu em 56 de 56 quadros.

1. **Capturas sem o jogador.** Roteiros do smoke test a partir dos saves do Memory card 1:
   - salas com legenda, prompt de item, seleção sim/não;
   - inventário, mapa, arquivos, tela de save, KEY CONFIG;
   - FMV, porta, game over, se der.
   Ampliar a cobertura com `ws_playtest` / `ws_analyze` até o separador acertar todas as cenas.
2. **Implementação** (patch de runtime próprio):
   - a cena é ampliada para preencher 16:9, com deslocamento vertical que segue a Jill e limites por câmera;
   - a interface é desenhada 1:1 por cima;
   - as telas 2D cheias ficam em 4:3 com barras laterais;
   - FMV fica em 4:3.
3. **Launcher:** opção "Widescreen (ampliar a cena)" ao lado do 16:9 esticado atual. O padrão só muda depois do teste do mantenedor.
4. **Aceite:**
   - capturas de todas as cenas do item 1, sem interface cortada nem cena distorcida;
   - smoke test OK, sem perda de FPS;
   - a Jill fica sempre visível no recorte (checagem pela posição dela na tela).

## Fase 4: controles modernos (slot 2 do Memory card 1)

1. Carregar o slot 2 pelo smoke test e reproduzir o problema com analógico sintético (`set_input lx/ly`) ao longo do corredor, atravessando os cortes de câmera. O `controls.csv` registra cada decisão por quadro.
2. **Hipóteses a testar:**
   - o "para a frente" é o vetor câmera → Jill;
   - o modo tanque entra de reserva nas rotinas acima de 4;
   - falta segurar a direção no corte de câmera.
3. **Aceite:** um trajeto roteirizado ida e volta pelo corredor sem inversão (a direção da Jill acompanha o analógico em todos os quadros fora da zona morta).
4. **Controle do Xbox:** não é necessário para o roteiro, porque a entrada é injetada. Ele só serve para o teste final do mantenedor, que decide se os controles modernos viram padrão.

## Fase 5: problemas menores

- **Texto de introdução rápido demais:** medir o tempo de cada tela (quadros) contra o original e corrigir se for causado pelo runtime.
- **`switch` do TITLE.BIN, quarta forma:** ensinar a forma ao reconhecedor (C++ e Python, com testes), regenerar e conferir que zera as ~1,9 mil instruções interpretadas.

## Fase 6: preparação da v2.0.0 (sem publicar)

1. Smoke test e teste do launcher passando no commit final.
2. **Roteiro de teste do mantenedor:**
   - fonte nítida;
   - widescreen em 3 salas, inventário, mapa e arquivos;
   - corredor dos controles;
   - PGXP, se ligado.
3. README nos 3 idiomas e notas da versão: o que mudou, o que não foi testado e os problemas conhecidos.
4. `manage_project -Action package -Version v2.0.0` só gera o zip local. A publicação depende do "sim" do mantenedor.

## Decisões que ficam com o mantenedor

- Responder as issues.
- Modelo de distribuição (gerar o código na máquina do jogador ou distribuir o `.exe`).
- Padrões da v2.0.0: widescreen em camadas, controles modernos e PGXP.
- Publicar a release.
