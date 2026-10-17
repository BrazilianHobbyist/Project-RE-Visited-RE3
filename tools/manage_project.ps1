# ============================================================
# Project RE: Visited - Resident Evil 3
# Script de Auditoria, Consolidacao, Organizacao e Release
# ============================================================
param(
    [ValidateSet("audit", "consolidate", "package", "publish")]
    [string]$Action = "audit",
    [string]$Version = "",
    [string]$ReleaseNotes = ""
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
    $gameExes = @(
        "$RepoDir\build\Resident_Evil_3__Nemesis_Recompiled.exe",
        "$PCPortDir\Resident_Evil_3__Nemesis_Recompiled.exe",
        "$WorkspaceRoot\Resident_Evil_3__Nemesis_Recompiled.exe"
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
    if (($gameHashes | Select-Object -Unique).Count -eq 1 -and $gameHashes.Count -eq 3) {
        Write-Host "[OK] Todos os 3 executaveis do jogo sao 100% identicos!" -ForegroundColor Green
    } else {
        Write-Host "[!] Divergencia detectada entre os executaveis do jogo!" -ForegroundColor Red
    }

    $launcherExes = @(
        "$RepoDir\launcher\Resident_Evil_3_Launcher.exe",
        "$PCPortDir\Resident_Evil_3_Launcher.exe",
        "$WorkspaceRoot\launcher\Resident_Evil_3_Launcher.exe"
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
    if (($launcherHashes | Select-Object -Unique).Count -eq 1 -and $launcherHashes.Count -eq 3) {
        Write-Host "[OK] Todos os 3 executaveis do Launcher sao 100% identicos!" -ForegroundColor Green
    } else {
        Write-Host "[!] Divergencia detectada entre os executaveis do Launcher!" -ForegroundColor Red
    }

    Write-Section "3. AUDITORIA DE TELEMETRIA E DUMPS DE FREEZE"
    $dumps = @(Get-ChildItem -Path $RepoDir, $PCPortDir, $WorkspaceRoot -Filter "psx_freeze_dump*.json" -ErrorAction SilentlyContinue)
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

    # 1. Atualizar o arquivo de patch do runtime a partir do submodule psxrecomp
    Write-Host "[1/5] Exportando diff atual do psxrecomp para patches/psxrecomp_runtime.patch..." -ForegroundColor Cyan
    $diffLines = git -C "$RepoDir\psxrecomp" diff
    [System.IO.File]::WriteAllLines("$RepoDir\patches\psxrecomp_runtime.patch", $diffLines, (New-Object System.Text.UTF8Encoding($false)))

    # 2. Sincronizar executavel compilado do jogo
    $builtExe = "$RepoDir\build\Resident_Evil_3__Nemesis_Recompiled.exe"
    if (Test-Path $builtExe) {
        Write-Host "[2/5] Sincronizando Resident_Evil_3__Nemesis_Recompiled.exe..." -ForegroundColor Cyan
        Copy-Item $builtExe "$PCPortDir\Resident_Evil_3__Nemesis_Recompiled.exe" -Force
        Copy-Item $builtExe "$WorkspaceRoot\Resident_Evil_3__Nemesis_Recompiled.exe" -Force
    }

    # 3. Compilar e sincronizar o Launcher
    Write-Host "[3/5] Compilando e sincronizando o Launcher..." -ForegroundColor Cyan
    & powershell -ExecutionPolicy Bypass -File "$RepoDir\launcher\build.ps1"

    # 4. Sincronizar documentacao (READMEs, CREDITS.md, PROGRESS.md, WORKFLOW.md)
    Write-Host "[4/5] Sincronizando documentacoes..." -ForegroundColor Cyan
    $docs = @("README.md", "README.pt-br.md", "README.es.md", "CREDITS.md", "PROGRESS.md", "WORKFLOW.md")
    foreach ($doc in $docs) {
        $srcDoc = Join-Path $RepoDir $doc
        if (Test-Path $srcDoc) {
            Copy-Item $srcDoc (Join-Path $WorkspaceRoot $doc) -Force
            if ($doc -ne "PROGRESS.md" -and $doc -ne "WORKFLOW.md") {
                Copy-Item $srcDoc (Join-Path $PCPortDir $doc) -Force
            }
        }
    }

    # 5. Limpar dumps de telemetria antigos
    Write-Host "[5/5] Removendo dumps temporarios de diagnostico (psx_freeze_dump*.json)..." -ForegroundColor Cyan
    Get-ChildItem -Path $RepoDir, $PCPortDir, $WorkspaceRoot -Filter "psx_freeze_dump*.json" -ErrorAction SilentlyContinue | Remove-Item -Force

    Write-Host "[OK] Consolidacao e sanitizacao concluidas com sucesso!" -ForegroundColor Green
}

function Invoke-Package {
    if ([string]::IsNullOrWhiteSpace($Version)) {
        throw "Informe a versao para empacotamento (ex: -Version 'v1.4.4')"
    }
    Write-Section "EMPACOTANDO RELEASE $Version (ZERO-ASSET POLICY)"
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
    Copy-Item "$PCPortDir\game.toml" "$stagingDir\" -Force
    Copy-Item "$PCPortDir\settings.toml" "$stagingDir\" -Force
    Copy-Item "$PCPortDir\input.ini" "$stagingDir\" -Force
    Copy-Item "$PCPortDir\re3.ico" "$stagingDir\" -Force
    Copy-Item "$RepoDir\CREDITS.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\README.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\README.pt-br.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\README.es.md" "$stagingDir\" -Force
    Copy-Item "$RepoDir\assets\banner.jpg" "$stagingDir\assets\banner.jpg" -Force
    Copy-Item "$PCPortDir\bios\openbios.bin" "$stagingDir\bios\openbios.bin" -Force
    Copy-Item "$PCPortDir\bios\OpenBIOS.LICENSE" "$stagingDir\bios\OpenBIOS.LICENSE" -Force
    if (Test-Path "$PCPortDir\mods\state.toml") {
        Copy-Item "$PCPortDir\mods\state.toml" "$stagingDir\mods\state.toml" -Force
    }

    # Verificacao estrita Zero-Asset Policy (Proibido incluir SLUS_009.23, .bin, .iso, SCPH*.BIN)
    $forbidden = Get-ChildItem -Path $stagingDir -Recurse | Where-Object {
        $_.Name -match "SLUS|SCPH|\.iso$|\.cue$" -or ($_.Extension -eq ".bin" -and $_.Name -ne "openbios.bin")
    }
    if ($forbidden) {
        Remove-Item -Recurse -Force $stagingDir
        throw "VIOLACAO DE ZERO-ASSET POLICY DETECTADA: $($forbidden.FullName -join ', ')"
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
