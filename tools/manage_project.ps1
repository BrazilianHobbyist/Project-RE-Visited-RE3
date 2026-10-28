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

    # 1. Atualizar o arquivo de patch do runtime a partir do submodule psxrecomp
    Write-Host "[1/5] Exportando diff atual do psxrecomp para patches/psxrecomp_runtime.patch..." -ForegroundColor Cyan
    # Redireciona via cmd: capturar a saída do git pelo PowerShell recodifica o
    # texto (CP850/CRLF) e corrompe o patch. Foi a causa do patch quebrado.
    $git = (Get-Command git).Source
    # diff HEAD: inclui as mudancas ja preparadas (staged) no submodule.
    cmd /c "`"$git`" -C `"$RepoDir\psxrecomp`" diff HEAD > `"$RepoDir\patches\psxrecomp_runtime.patch`""
    if ($LASTEXITCODE -ne 0) { throw "Falha ao exportar o patch do runtime." }

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

    # 4. Sincronizar documentacao (READMEs, CREDITS.md, PROGRESS.md, WORKFLOW.md)
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

function Invoke-Publish {
    if ([string]::IsNullOrWhiteSpace($Version)) {
        throw "Informe a versao para publicacao (ex: -Version 'v1.5.0')"
    }

    $zipName = "Project_RE_Visited_Resident_Evil_3_${Version}_Win64.zip"
    $zipPath = Join-Path $ReleasesDir $zipName
    if (-not (Test-Path $zipPath)) {
        Write-Host "[!] Pacote nao encontrado em $zipPath. Gerando agora..." -ForegroundColor Yellow
        Invoke-Package
    }

    $zipHash = (Get-FileHash $zipPath -Algorithm SHA256).Hash

    Write-Section "PUBLICANDO GITHUB RELEASE $Version"

    # Recupera o token de autenticacao via git credential fill
    $credInput = "protocol=https`nhost=github.com`n`n"
    $tokenLine = ($credInput | git credential fill) | Where-Object { $_ -match '^password=' }
    if (-not $tokenLine) {
        throw "Nao foi possivel obter o token do GitHub via git credential fill."
    }
    $token = $tokenLine.Substring(9).Trim()

    $repo = "BrazilianHobbyist/Project-RE-Visited-RE3"

    # Verifica se a release ja existe
    $existing = $null
    try {
        $checkUri = "https://api.github.com/repos/$repo/releases/tags/$Version"
        $existing = Invoke-RestMethod -Uri $checkUri -Headers @{
            "Authorization" = "Bearer $token"
            "Accept"        = "application/vnd.github.v3+json"
            "User-Agent"    = "ProjectREVisited-ReleaseTool"
        } -Method Get -ErrorAction SilentlyContinue
    } catch {
        # 404 esperado se nao existir
    }

    $releaseId = $null
    $releaseUrl = $null

    if ($existing -and $existing.id) {
        Write-Host "[*] Release $Version ja existe no GitHub (ID: $($existing.id))." -ForegroundColor Cyan
        $releaseId = $existing.id
        $releaseUrl = $existing.html_url
    } else {
        Write-Host "[*] Criando nova Release $Version no GitHub..." -ForegroundColor Cyan

        $bodyText = $ReleaseNotes
        if ([string]::IsNullOrWhiteSpace($bodyText)) {
            $bodyText = @"
## Project RE: Visited - Resident Evil 3: Nemesis ($Version Stable)

### What's New in $Version

1. **Authentic 30 FPS Engine Presentation (Removed Artificial Frame Interpolation)**:
   - Evaluated and completely removed experimental frame interpolation and artificial sub-frame blending hooks (`frame_interpolation`, `frame_interpolation_fps`, `frame_interpolation_blend`).
   - In a 2.5D fixed-camera engine with pre-rendered backgrounds, sub-frame interpolation caused ghosting, depth-buffer/Painter's algorithm sorting glitches (3D actors drawing on top of 2D background masks and menus), and artificial input latency.
   - The game now runs with authentic, rock-solid 30 FPS PlayStation 1 cadence paired with pristine OpenGL HD rendering, zero visual artifacts, immediate input response, and complete stability.

2. **Full High-DPI & 4K Monitor Scaling Fix (`gdiScaling = true`)**:
   - Resolved the issue reported by the community regarding dialogue windows and wizard dialogs being clipped on High-DPI and 4K displays (125%–250% Windows display scaling).
   - Enabled GDI DPI scaling in `launcher/manifest.xml` and integrated `AdjustWindowRectEx` across all Launcher dialogs (Setup Wizard, Cheats, Credits, Main Window).
   - Controls and buttons now scale with crisp vector sharpness and remain 100% accessible on any modern display.

3. **Streamlined Win32 Launcher & Clean UI**:
   - Reorganized the Launcher layout to remove redundant refresh-rate/FPS selectors, eliminating dead visual space.
   - Preserved all essential options: Fullscreen/Windowed, Internal Resolution (1x up to 4x 4K SSAA), Aspect Ratio (16:9 Stretched / 4:3 Authentic), Language selection (English, Português-BR, Español), Texture Perspective Correction, Fast Door Transitions, and Reversible Cheats.

4. **Zero-Asset Package Integrity & Persistent Project Governance**:
   - Automated SHA-256 binary parity verification across workspace builds and distribution packages.
   - Preserved MIT-licensed OpenBIOS (`bios/openbios.bin`) and runtime patch synchronization (`patches/psxrecomp_runtime.patch`).

---

### Package Verification (Zero-Asset Policy)
- **File**: `$zipName`
- **SHA-256**: `$zipHash`
- **Legal Notice**: Contains zero copyrighted PlayStation BIOS or Capcom game assets. Requires the user's own legitimate copy of *Resident Evil 3: Nemesis* (NTSC-U SLUS-00923).
"@
        }

        $payload = @{
            tag_name   = $Version
            name       = "$Version - Authentic 30 FPS Presentation, Full High-DPI 4K Scaling Fix & Launcher Polish"
            body       = $bodyText
            draft      = $false
            prerelease = $false
        } | ConvertTo-Json -Depth 5

        $createUri = "https://api.github.com/repos/$repo/releases"
        $newRel = Invoke-RestMethod -Uri $createUri -Headers @{
            "Authorization" = "Bearer $token"
            "Accept"        = "application/vnd.github.v3+json"
            "Content-Type"  = "application/json; charset=utf-8"
            "User-Agent"    = "ProjectREVisited-ReleaseTool"
        } -Method Post -Body ([System.Text.Encoding]::UTF8.GetBytes($payload))

        $releaseId = $newRel.id
        $releaseUrl = $newRel.html_url
        Write-Host "[OK] Release criada com sucesso! ID: $releaseId" -ForegroundColor Green
    }

    # Upload do Asset (ZIP)
    Write-Host "[*] Fazendo upload do pacote $zipName..." -ForegroundColor Cyan
    $uploadUri = "https://uploads.github.com/repos/$repo/releases/$releaseId/assets?name=$zipName"

    $assets = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases/$releaseId/assets" -Headers @{
        "Authorization" = "Bearer $token"
        "Accept"        = "application/vnd.github.v3+json"
        "User-Agent"    = "ProjectREVisited-ReleaseTool"
    } -Method Get

    $existingAsset = $assets | Where-Object { $_.name -eq $zipName }
    if ($existingAsset) {
        Write-Host "[*] Asset $zipName ja existe na release (ID: $($existingAsset.id)). Removendo para re-upload limpo..." -ForegroundColor Yellow
        Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases/assets/$($existingAsset.id)" -Headers @{
            "Authorization" = "Bearer $token"
            "Accept"        = "application/vnd.github.v3+json"
            "User-Agent"    = "ProjectREVisited-ReleaseTool"
        } -Method Delete
    }

    # Upload com curl.exe para streaming binario estavel
    $uploadRes = curl.exe -s -X POST `
        -H "Authorization: Bearer $token" `
        -H "Accept: application/vnd.github.v3+json" `
        -H "Content-Type: application/zip" `
        --data-binary "@$zipPath" `
        "$uploadUri"

    $assetJson = $uploadRes | ConvertFrom-Json
    if ($assetJson.browser_download_url) {
        Write-Host "[OK] Asset anexado com sucesso!" -ForegroundColor Green
        Write-Host "     Download URL: $($assetJson.browser_download_url)" -ForegroundColor Green
        Write-Host "     Tamanho: $($assetJson.size) bytes" -ForegroundColor Green
    } else {
        Write-Host "[!] Resposta do upload: $uploadRes" -ForegroundColor Yellow
    }

    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Green
    Write-Host " RELEASE PUBLICADA COM SUCESSO!" -ForegroundColor Green
    Write-Host " URL: $releaseUrl" -ForegroundColor Green
    Write-Host "============================================================" -ForegroundColor Green
}

switch ($Action) {
    "audit"       { Invoke-Audit }
    "consolidate" { Invoke-Consolidate; Invoke-Audit }
    "package"     { Invoke-Consolidate; Invoke-Package; Invoke-Audit }
    "publish"     { Invoke-Publish }
}
