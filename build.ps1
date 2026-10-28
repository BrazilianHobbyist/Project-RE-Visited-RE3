# ============================================================
# RE3Recomp — Script de Build (Windows PowerShell)
# ============================================================

param(
    [string]$Config = "Release",
    [switch]$Clean,
    [switch]$Regen,     # Regenerar o código C a partir do binário
    [string]$DiscPath = ""
)

$ProjectRoot = $PSScriptRoot
$BuildDir    = "$ProjectRoot\build"
$GeneratedDir = "$BuildDir\generated"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " RE3Recomp Build Script" -ForegroundColor Cyan
Write-Host " Config: $Config" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

if (-not (Get-Command "git" -ErrorAction SilentlyContinue)) {
    if (Test-Path "$env:LOCALAPPDATA\Programs\GitPortable\cmd") {
        $env:PATH = "$env:LOCALAPPDATA\Programs\GitPortable\cmd;$env:PATH"
    }
}

# ---- Verificar pré-requisitos ----
function Test-Command($cmd) {
    return $null -ne (Get-Command $cmd -ErrorAction SilentlyContinue)
}

$missing = @()
if (-not (Test-Command "git"))    { $missing += "git" }
if (-not (Test-Command "cmake"))  { $missing += "cmake" }
if (-not (Test-Command "ninja"))  { $missing += "ninja" }
if (-not (Test-Command "python")) { $missing += "python" }

if ($missing.Count -gt 0) {
    Write-Host "[X] Ferramentas faltando: $($missing -join ', ')" -ForegroundColor Red
    Write-Host "   Instale as ferramentas e tente novamente." -ForegroundColor Red
    exit 1
}

# Verificar submodule PSXRecomp
if (-not (Test-Path "$ProjectRoot\psxrecomp\runtime\runtime.cmake")) {
    Write-Host "[INFO] PSXRecomp submodule não encontrado. Clonando..." -ForegroundColor Yellow
    & git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[X] Falha ao clonar submodule PSXRecomp" -ForegroundColor Red
        exit 1
    }
}

# Aplica um patch de forma idempotente: pula se já aplicado, falha se não aplicar.
function Apply-Patch($Dir, $PatchFile, [switch]$IgnoreWhitespace) {
    $ws = @(); if ($IgnoreWhitespace) { $ws = @("--ignore-whitespace") }
    $name = Split-Path $PatchFile -Leaf
    & git -C $Dir apply --check -R @ws "$PatchFile" 2>$null
    if ($LASTEXITCODE -eq 0) { Write-Host "[PATCH] $name já aplicado." -ForegroundColor DarkGray; return }
    & git -C $Dir apply --check @ws "$PatchFile" 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[X] $name não aplica em $Dir. Verifique a versão do código base." -ForegroundColor Red
        exit 1
    }
    Write-Host "[PATCH] Aplicando $name..." -ForegroundColor Cyan
    & git -C $Dir apply @ws "$PatchFile"
    if ($LASTEXITCODE -ne 0) { Write-Host "[X] Falha ao aplicar $name" -ForegroundColor Red; exit 1 }
}

# Patches do runtime (alterações RE3 sobre o PSXRecomp): patches/runtime/NN-*.patch,
# em ordem. tools/runtime_patches.py aplica num submodule limpo e avisa quando
# há edições de desenvolvimento além da sequência.
& python "$ProjectRoot\tools\runtime_patches.py" ensure
if ($LASTEXITCODE -ne 0) { Write-Host "[X] Patches do runtime não aplicam." -ForegroundColor Red; exit 1 }

# ---- Limpar build se solicitado ----
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[CLEAN] Limpando diretório de build..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

# ---- Regenerar código C a partir do binário (Fase 2+3) ----
if ($Regen) {
    if ($DiscPath -eq "") {
        Write-Host "[X] Use -DiscPath 'C:\caminho\para\RE3.cue' com -Regen" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "[REGEN] Executando recompilador PSXRecomp..." -ForegroundColor Cyan

    # Gera generated/ a partir do disco do usuário (game.toml define o destino).
    # As opções vêm depois do subcomando (CLI do upstream). Sem download de
    # toolchain: o build usa o compilador do sistema.
    & python "$ProjectRoot\psxrecomp\psxrecomp_cli.py" generate `
        --config "$ProjectRoot\game.toml" `
        --project-root "$ProjectRoot" `
        --disc "$DiscPath" `
        --no-toolchain-download

    if ($LASTEXITCODE -ne 0) {
        Write-Host "[X] Falha na recompilação. Verifique o game.toml e o disco." -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] Código C gerado em: $ProjectRoot\generated" -ForegroundColor Green
}

# Patches do código gerado (pulo de portas e silêncio de áudio nas portas), em ordem.
# O segundo depende do contexto do primeiro: se o último já está aplicado, a cadeia toda está.
if (Test-Path "$ProjectRoot\generated\SLUS_009.23_full_01.c") {
    $last = "$ProjectRoot\patches\re3_door_audio_mute.patch"
    & git -C $ProjectRoot apply --check -R --ignore-whitespace "$last" 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[PATCH] Patches do código gerado já aplicados." -ForegroundColor DarkGray
    } else {
        Apply-Patch $ProjectRoot "$ProjectRoot\patches\re3_door_fade_speedup.patch" -IgnoreWhitespace
        Apply-Patch $ProjectRoot $last -IgnoreWhitespace
    }
}

# ---- Configurar CMake ----
Write-Host "[CONFIG]  Configurando CMake..." -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

cmake -S "$ProjectRoot" `
      -B "$BuildDir" `
      -G Ninja `
      "-DCMAKE_BUILD_TYPE=$Config" `
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if ($LASTEXITCODE -ne 0) {
    Write-Host "[X] CMake configure falhou." -ForegroundColor Red
    exit 1
}

# ---- Build ----
# Build reproduzível: __DATE__/__TIME__ usam a data do commit, não a hora do build.
$commitTime = & git -C $ProjectRoot log -1 --format=%ct 2>$null
if ($LASTEXITCODE -eq 0 -and $commitTime) { $env:SOURCE_DATE_EPOCH = $commitTime.Trim() }
Write-Host "[BUILD] Compilando..." -ForegroundColor Cyan
cmake --build "$BuildDir" --config $Config -- -j (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

if ($LASTEXITCODE -ne 0) {
    Write-Host "[X] Build falhou." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[OK] Build concluído!" -ForegroundColor Green
Write-Host "   Executável: $BuildDir\Resident_Evil_3__Nemesis_Recompiled.exe" -ForegroundColor Green
Write-Host ""
Write-Host "Para executar:" -ForegroundColor Cyan
Write-Host "  .\build\Resident_Evil_3__Nemesis_Recompiled.exe" -ForegroundColor White
Write-Host ""
Write-Host "Atalhos de teclado:" -ForegroundColor Cyan
Write-Host "  Alt+Enter  -> Alternar fullscreen/janela" -ForegroundColor White
Write-Host "  F11        -> Fullscreen" -ForegroundColor White
