# Fluxo de trabalho

## Estrutura

- `RE3Recomp/` (este repositório): única fonte de verdade.
- `ResidentEvil3-PCPort/` (fora do repositório): única pasta de execução e testes. Contém disco, BIOS, saves e as configurações pessoais do mantenedor.
- `releases/` (fora do repositório): zips gerados localmente.

Não existem outras cópias do executável ou dos documentos.

## Alterar o runtime (submodule `psxrecomp`)

1. Edite os arquivos em `psxrecomp/`.
2. `.\build.ps1` (avisa que o submodule tem mudanças além de `patches/runtime`; é esperado).
3. Antes de commitar, grave a mudança num patch por tema, com `tools\runtime_patches.py`, que escreve os bytes direto do git (o PowerShell recodifica a saída e corrompe patches):
   - assunto novo: `py -3 tools\runtime_patches.py export NN-tema` (vira o último da sequência);
   - mudança num tema existente: `py -3 tools\runtime_patches.py rebuild NN-tema` (reescreve só aquele; os seguintes precisam continuar aplicando).
   `status` deve terminar em `applied`. Os patches aplicam em ordem de nome sobre o commit do submódulo:

   | Patch | Conteúdo |
   |---|---|
   | `01-recompiler-switch-form` | terceira forma de `switch` limitado (C++, Python, testes) |
   | `02-pgxp` | solda de vértices, rastreio de deslocamento, profundidade pelo plano, porta "jogo começou" |
   | `03-presentation` | 16:9 esticado, FXAA, recorte do depth24 em FMV, janela em primeiro plano |
   | `04-doors-audio` | portas rápidas: fronteira de carga e fade do áudio |
   | `05-settings-input-cheats` | `settings.toml` do RE3, controles modernos, cheats, vibração, DirectInput |
   | `06-diagnostics` | dados por quadro, histórico, histograma do interpretador, vigia de travamento |
   | `07-misc` | memória do SHA do disco, clamp de draw distance |

   Se a mudança for no recompilador (`recompiler/src/`), a etiqueta do cache muda. Regenere o C (`.\build.ps1 -Regen -DiscPath <cue>`) e o cache de overlays (veja abaixo).
4. Commite os patches em `RE3Recomp`. O submodule continua apontando para o commit original do PSXRecomp.

## Alterar o código gerado (`generated/`)

`generated/` não é versionado (vem do disco do usuário). Mudanças nele são patches em `patches/re3_*.patch`, aplicados pelo `build.ps1` na ordem: `re3_door_fade_speedup.patch`, depois `re3_door_audio_mute.patch`.

## Cache de overlays (a partir do disco)

O cache que vai no pacote é gerado pelo pipeline AOT do upstream, a partir do disco. Nunca vem de capturas de jogo.

```
py -3 tools\make_aot_profile.py --cue <cue>     # só se o perfil precisar mudar
py -3 psxrecomp\tools\aot_overlay_pipeline.py release --profile aot\overlays.json --game-toml game.toml `
   --disc <cue> --recompiler psxrecomp\recompiler\build\psxrecomp-game.exe `
   --runtime-build-dir build --runtime-target RE3Recomp --cps --gcc <gcc> `
   --work-dir <tmp> --stage <tmp>\stage
```

Copie `<tmp>\stage\cache\...\<etiqueta>` para `ResidentEvil3-PCPort\cache\SLUS-00923\gcc\win-x64\`. A auditoria (`AOT_CACHE_AUDIT.json`) precisa dar `all_pairs_valid: true`.

## Testar

`tools\manage_project.ps1 -Action consolidate` copia o build para `ResidentEvil3-PCPort`. Teste pelo launcher daquela pasta. Uma funcionalidade só é "funciona" no README depois de testada no jogo.

Teste automático (sem jogador): `py -3 tools\smoke_test.py`. Ele abre o build de debug, vai do boot ao título, carrega o primeiro save do Memory card 1, anda com o direcional e com o analógico, abre e fecha o inventário e confere tudo pela RAM. O resultado sai em `captures\smoke_<data>` (capturas numeradas, `stdout.log` e `report.json`, com FPS, underruns de áudio e `no_depth` da correção de textura); o código de saída é 0 só se todas as checagens passarem. Os cartões do jogador nunca são escritos: o jogo roda com `--memcard-dir` numa cópia dentro da sessão. O script recusa rodar se já houver um RE3 aberto ou se a porta 4370 estiver ocupada. Abra as capturas: elas são a evidência, e as checagens não veem defeitos visuais. As capturas saem a 1920×1080 (`--window`); numa janela pequena, diferenças de textura somem. `--hd-pack <pasta>` roda com um pacote de texturas HD (`--hd-dump` também extrai texturas); o `mods\state.toml` da pasta de execução é trocado e restaurado no fim. O jogo usa o `settings.toml` da pasta de execução.

Fonte nítida: `py -3 tools\test_launcher_sharp_font.py --dumps <pasta de dumps>` dirige o launcher real num sandbox em `captures\` (marcar, extrair, gerar, desmarcar). `tools\make_font_upscale.py` é o protótipo em Python do mesmo EPX.

Para investigar com o servidor de debug (TCP 4370), use `.\tools\run_debug.ps1` (`-Build` recompila o `build_dbg`). O build de release não tem servidor. Veja `docs\WIDESCREEN_PLAYTEST.md`.

## Release

1. `py -3 tools\smoke_test.py` com o `build_dbg` do mesmo commit (`tools\run_debug.ps1 -Build -NoCapture`, depois feche o jogo): precisa terminar com `"result": "ok"`.
2. `tools\manage_project.ps1 -Action audit`: confira git limpo e build igual ao da pasta de execução.
3. `tools\manage_project.ps1 -Action package -Version vX.Y.Z`: o zip usa `game.toml` e `settings.default.toml` do repositório, nunca as configurações da pasta de testes.
4. Publique com o GitHub CLI (`gh release create`), com notas que digam o que mudou, o que não foi testado e os problemas conhecidos.
