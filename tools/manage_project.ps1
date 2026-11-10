# ============================================================
# Project RE: Visited - Resident Evil 3
# Script de Auditoria, Consolidacao, Organizacao e Release
# ============================================================
param(
    [ValidateSet("audit", "consolidate", "package")]
    [string]$Action = "audit",
    [string]$Version = ""
)

$ErrorActionPreference = "Stop"
$WorkspaceRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if (-not (Test-Path "$WorkspaceRoot\RE3Recomp")) {
    $WorkspaceRoot = Split-Path $PSScriptRoot -Parent
}

$RepoDir     = Join-Path $WorkspaceRoot "RE3Recomp"
$PCPortDir   = Join-Path $WorkspaceRoot "ResidentEvil3-PCPort"
$ReleasesDir = Join-Path $WorkspaceRoot "releases"

if (-not (Get-Command "git" -ErrorAction SilentlyContinue)) {
    if (Test-Path "$env:LOCALAPPDATA\Programs\GitPortable\cmd") {
        $env:PATH = "$env:LOCALAPPDATA\Programs\GitPortable\cmd;$env:PATH"
    }
}

function Write-Section($title) {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Cyan
    Write-Host " $title" -ForegroundColor Cyan
    Write-Host "============================================================" -ForegroundColor Cyan
}

function Invoke-Audit {
    Write-Section "1. AUDITORIA DE GIT & SUBMODULO (RE3Recomp)"
    $gitStatus = git -C $RepoDir status --short
    if ($gitStatus) {
        Write-Host "[!] Alteracoes nao commitadas em RE3Recomp:" -ForegroundColor Yellow
        $gitStatus | ForEach-Object { Write-Host "    $_" -ForegroundColor Yellow }
    } else {
        Write-Host "[OK] Repositorio RE3Recomp limpo (sem modificacoes pendentes)." -ForegroundColor Green
    }

    $subDiff = git -C "$RepoDir\psxrecomp" diff
    Write-Host "    Submodule psxrecomp linhas de diff: $($subDiff.Count)"

    Write-Section "2. AUDITORIA DE PARIDADE DE BINARIOS (SHA-256)"
    # Uma única pasta de execução (ResidentEvil3-PCPort); o build deve bater com ela.
    $gameExes = @(
        "$RepoDir\build\Resident_Evil_3__Nemesis_Recompiled.exe",
        "$PCPortDir\Resident_Evil_3__Nemesis_Recompiled.exe"
    )
    $gameHashes = @()
    foreach ($p in $gameExes) {
        if (Test-Path $p) {
            $h = (Get-FileHash $p -Algorithm SHA256).Hash
            $gameHashes += $h
            Write-Host "    [EXE Jogo] $($h.Substring(0,16))... -> $p"
        } else {
            Write-Host "    [AUSENTE] $p" -ForegroundColor Red
        }
    }
    if (($gameHashes | Select-Object -Unique).Count -eq 1 -and $gameHashes.Count -eq $gameExes.Count) {
        Write-Host "[OK] Build e pasta de execucao identicos." -ForegroundColor Green
    } else {
        Write-Host "[!] Divergencia detectada entre os executaveis do jogo!" -ForegroundColor Red
    }

    $launcherExes = @(
        "$RepoDir\launcher\Resident_Evil_3_Launcher.exe",
        "$PCPortDir\Resident_Evil_3_Launcher.exe"
    )
    $launcherHashes = @()
    foreach ($p in $launcherExes) {
        if (Test-Path $p) {
            $h = (Get-FileHash $p -Algorithm SHA256).Hash
            $launcherHashes += $h
            Write-Host "    [Launcher] $($h.Substring(0,16))... -> $p"
        } else {
            Write-Host "    [AUSENTE] $p" -ForegroundColor Red
        }
    }
    if (($launcherHashes | Select-Object -Unique).Count -eq 1 -and $launcherHashes.Count -eq $launcherExes.Count) {
        Write-Host "[OK] Launcher do build e da pasta de execucao identicos." -ForegroundColor Green
    } else {
        Write-Host "[!] Divergencia detectada entre os executaveis do Launcher!" -ForegroundColor Red
    }

    Write-Section "3. AUDITORIA DE TELEMETRIA E DUMPS DE FREEZE"
    $dumps = @(Get-ChildItem -Path $RepoDir, $PCPortDir -Filter "psx_freeze_dump*.json" -ErrorAction SilentlyContinue)
    if ($dumps.Count -gt 0) {
        Write-Host "[!] Encontrados $($dumps.Count) arquivo(s) de freeze dump:" -ForegroundColor Yellow
        foreach ($d in $dumps) {
            Write-Host "    - $($d.FullName) ($($d.LastWriteTime))" -ForegroundColor Yellow
        }
    } else {
        Write-Host "[OK] Nenhum arquivo residual de freeze dump encontrado." -ForegroundColor Green
    }

    $lastReport = Join-Path $PCPortDir "psx_last_run_report.json"
    if (Test-Path $lastReport) {
        $rep = Get-Content $lastReport -Raw | ConvertFrom-Json
        Write-Host "    Ultimo Run Report: reason=$($rep.reason), exit_origin=$($rep.exit_origin), frames=$($rep.frame), timestamp=$($rep.timestamp)"
    }
}

function Invoke-Consolidate {
    Write-Section "CONSOLIDANDO E ORGANIZANDO O PROJETO"

    # 1. Conferir que patches/runtime reproduz o submodule (a exportacao e manual,
    #    por tema: tools/runtime_patches.py export|rebuild)
    Write-Host "[1/5] Conferindo patches/runtime contra o psxrecomp..." -ForegroundColor Cyan
    $st = & python "$RepoDir\tools\runtime_patches.py" status
    if ($st -and $st[0] -eq "dirty") {
        throw "psxrecomp tem mudancas fora de patches/runtime. Use tools\runtime_patches.py export|rebuild."
    }

    # 2. Sincronizar executavel compilado do jogo
    $builtExe = "$RepoDir\build\Resident_Evil_3__Nemesis_Recompiled.exe"
    if (Test-Path $builtExe) {
        Write-Host "[2/5] Sincronizando Resident_Evil_3__Nemesis_Recompiled.exe..." -ForegroundColor Cyan
        Copy-Item $builtExe "$PCPortDir\Resident_Evil_3__Nemesis_Recompiled.exe" -Force
    }
    # Carregador de texturas HD (opcao "Fonte nitida" do launcher). So este pacote:
    # os outros de mods/bundled ainda nao foram testados no RE3.
    $hdPkg = "$RepoDir\build\mods\bundled\psx.enhancement.hd-textures"
    if (Test-Path $hdPkg) {
        New-Item -ItemType Directory -Force -Path "$PCPortDir\mods\bundled" | Out-Null
        Copy-Item $hdPkg "$PCPortDir\mods\bundled\" -Recurse -Force
    }

    # 3. Compilar e sincronizar o Launcher
    Write-Host "[3/5] Compilando e sincronizando o Launcher..." -ForegroundColor Cyan
    & powershell -ExecutionPolicy Bypass -File "$RepoDir\launcher\build.ps1"

    # 4. Sincronizar documentacao (READMEs, CREDITS.md)
    Write-Host "[4/5] Sincronizando documentacoes..." -ForegroundColor Cyan
    foreach ($doc in @("README.md", "README.pt-br.md", "README.es.md", "CREDITS.md")) {
        $srcDoc = Join-Path $RepoDir $doc
        if (Test-Path $srcDoc) { Copy-Item $srcDoc (Join-Path $PCPortDir $doc) -Force }
    }

    # 5. Limpar dumps de telemetria antigos
    Write-Host "[5/5] Removendo dumps temporarios de diagnostico (psx_freeze_dump*.json)..." -ForegroundColor Cyan
    Get-ChildItem -Path $RepoDir, $PCPortDir -Filter "psx_freeze_dump*.json" -ErrorAction SilentlyContinue | Remove-Item -Force

    Write-Host "[OK] Consolidacao e sanitizacao concluidas com sucesso!" -ForegroundColor Green
}

function Invoke-Package {
    if ([string]::IsNullOrWhiteSpace($Version)) {
        throw "Informe a versao para empacotamento (ex: -Version 'v1.4.4')"
    }
    Write-Section "EMPACOTANDO RELEASE $Version"
    New-Item -ItemType Directory -Force -Path $ReleasesDir | Out-Null

    $zipName = "Project_RE_Visited_Resident_Evil_3_${Version}_Win64.zip"
    $zipPath = Join-Path $ReleasesDir $zipName
    $stagingDir = Join-Path $env:TEMP "RE3_Release_Staging_$Version"

    if (Test-Path $stagingDir) { Remove-Item -Recurse -Force $stagingDir }
    New-Item -ItemType Directory -Force -Path $stagingDir | Out-Null
    New-Item -ItemType Directory -Force -Path "$stagingDir\assets" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stagingDir\bios" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stagingDir\mods" | Out-Null

    Copy-Item "$PCPortDir\Resident_Evil_3__Nemesis_Recompiled.exe" "$stagingDir\" -Force
    Copy-Item "$PCPortDir\Resident_Evil_3_Launcher.exe" "$stagingDir\" -Force
    # Configurações padrão do repositório, nunca as da pasta de testes do mantenedor.
    Copy-Item "$RepoDir\game.toml" "$stagingDir\" -Force
    Copy-Item "$RepoDir\settings.default.toml" "$stagingDir\settings.toml" -Force
    Copy-Item "$PCPortDir\input.ini" "$stagingDir\" -Force
    Copy-Item "$PCPortDir\re3.ico" "$stagingDir\" -Force
    Copy-Item "$RepoDir\CREDITS.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\README.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\README.pt-br.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\README.es.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\assets\banner.jpg" "$stagingDir\assets\banner.jpg" -Force
    Copy-Item "$PCPortDir\bios\openbios.bin" "$stagingDir\bios\openbios.bin" -Force
    Copy-Item "$PCPortDir\bios\OpenBIOS.LICENSE" "$stagingDir\bios\OpenBIOS.LICENSE" -Force
    # state.toml limpo: o da pasta de testes guarda escolhas e caminhos absolutos
    # do mantenedor (ex.: o pacote da fonte nítida).
    [System.IO.File]::WriteAllText("$stagingDir\mods\state.toml", "format_version = 2`n")
    # Só o manifesto do carregador de texturas HD; nenhuma textura vai no pacote
    # (a fonte nítida é gerada na máquina do jogador, a partir do jogo dele).
    New-Item -ItemType Directory -Force -Path "$stagingDir\mods\bundled" | Out-Null
    Copy-Item "$RepoDir\build\mods\bundled\psx.enhancement.hd-textures" "$stagingDir\mods\bundled\" -Recurse -Force

    # Overlays: cache nativo pre-compilado + kit de compilacao (Python embutido,
    # TinyCC, recompilador). Sem eles o codigo das salas roda interpretado na
    # maquina do jogador. Usa as funcoes oficiais do PSXRecomp.
    $fw = Join-Path $RepoDir "psxrecomp"
    . (Join-Path $fw "tools\release_overlay_stage.ps1")
    $recompDir = Join-Path $fw "recompiler\build"
    $cgTag = Get-OverlayCgTag -RecompTools "$fw\tools" -RecompInc "$fw\runtime\include" `
        -GameExe "$recompDir\psxrecomp-game.exe" -GameToml "$stagingDir\game.toml" `
        -BuildPath "$RepoDir\build" -RuntimeTarget "RE3Recomp"
    Write-Host "    Overlay cache tag: $cgTag"
    $shards = Add-OverlayCache -GameId "SLUS-00923" -CacheSrcRoot "$PCPortDir\cache" `
        -Stage $stagingDir -CgTag $cgTag
    Write-Host "    Shards de overlay incluidos: $shards"
    Add-OverlayToolchain -Stage $stagingDir -RecompDir $recompDir -RecompTools "$fw\tools" `
        -RecompInc "$fw\runtime\include" -MingwBin $recompDir `
        -DlCache (Join-Path $WorkspaceRoot ".dl_cache") | Out-Null

    # Nada de midia do disco, BIOS da Sony ou capturas de RAM (que contem bytes
    # do jogo). O exe e os shards contem codigo do jogo traduzido; isso e
    # declarado no README/CREDITS.
    $forbidden = Get-ChildItem -Path $stagingDir -Recurse -File | Where-Object {
        $_.Name -match "^SLUS_\d{3}\.\d{2}$|^SCPH.*\.BIN$|\.iso$|\.cue$|\.chd$|overlay_captures|\.mcd$|ramdump" -or
        ($_.Extension -eq ".bin" -and $_.Name -ne "openbios.bin")
    }
    if ($forbidden) {
        Remove-Item -Recurse -Force $stagingDir
        throw "ARQUIVO PROIBIDO NO PACOTE: $($forbidden.FullName -join ', ')"
    }

    if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
    Compress-Archive -Path "$stagingDir\*" -DestinationPath $zipPath -CompressionLevel Optimal
    Remove-Item -Recurse -Force $stagingDir

    $hash = (Get-FileHash $zipPath -Algorithm SHA256).Hash
    Write-Host "[OK] Pacote gerado: $zipPath" -ForegroundColor Green
    Write-Host "     SHA-256: $hash" -ForegroundColor Green
}

switch ($Action) {
    "audit"       { Invoke-Audit }
    "consolidate" { Invoke-Consolidate; Invoke-Audit }
    "package"     { Invoke-Consolidate; Invoke-Package; Invoke-Audit }
}
