# Fluxo de trabalho

## Estrutura

- `RE3Recomp/` (este repositório): única fonte de verdade.
- `ResidentEvil3-PCPort/` (fora do repositório): única pasta de execução e testes. Contém disco, BIOS, saves e as configurações pessoais do mantenedor.
- `releases/` (fora do repositório): zips gerados localmente.

Não existem outras cópias do executável ou dos documentos.

## Alterar o runtime (submodule `psxrecomp`)

1. Edite os arquivos em `psxrecomp/`.
2. `.\build.ps1` (avisa em amarelo que o submodule tem mudanças além do patch; é esperado).
3. Antes de commitar, exporte o patch **pelo cmd**, nunca capturando a saída do git no PowerShell (isso corrompe acentos e o BOM):
   ```
   cmd /c "git -C psxrecomp diff > patches\psxrecomp_runtime.patch"
   ```
4. Commite o patch em `RE3Recomp`. O submodule continua apontando para o commit original do PSXRecomp.

## Alterar o código gerado (`generated/`)

`generated/` não é versionado (vem do disco do usuário). Mudanças nele são patches em `patches/re3_*.patch`, aplicados pelo `build.ps1` na ordem: `re3_door_fade_speedup.patch`, depois `re3_door_audio_mute.patch`.

## Testar

`tools\manage_project.ps1 -Action consolidate` copia o build para `ResidentEvil3-PCPort`. Teste pelo launcher daquela pasta. Uma funcionalidade só é "funciona" no README depois de testada no jogo.

## Release

1. `tools\manage_project.ps1 -Action audit`: confira git limpo e build igual ao da pasta de execução.
2. `tools\manage_project.ps1 -Action package -Version vX.Y.Z`: o zip usa `game.toml` e `settings.default.toml` do repositório, nunca as configurações da pasta de testes.
3. Publique com o GitHub CLI (`gh release create`), com notas que digam o que mudou, o que não foi testado e os problemas conhecidos.
