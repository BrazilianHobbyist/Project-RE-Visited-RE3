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
    Write-Host "❌ Ferramentas faltando: $($missing -join ', ')" -ForegroundColor Red
    Write-Host "   Instale as ferramentas e tente novamente." -ForegroundColor Red
    exit 1
}

# Verificar submodule PSXRecomp
if (-not (Test-Path "$ProjectRoot\psxrecomp\CMakeLists.txt")) {
    Write-Host "📦 PSXRecomp submodule não encontrado. Clonando..." -ForegroundColor Yellow
    & git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Falha ao clonar submodule PSXRecomp" -ForegroundColor Red
        exit 1
    }
}

# Aplicar patches customizados no runtime (Interpolação de Movimento, Controles 3D, Widescreen)
$PatchFile = "$ProjectRoot\patches\psxrecomp_runtime.patch"
if (Test-Path $PatchFile) {
    $canApply = & git -C "$ProjectRoot\psxrecomp" apply --check "$PatchFile" 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "🔧 Aplicando patches customizados no PSXRecomp runtime..." -ForegroundColor Cyan
        & git -C "$ProjectRoot\psxrecomp" apply "$PatchFile"
    }
}

# ---- Limpar build se solicitado ----
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "🧹 Limpando diretório de build..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

# ---- Regenerar código C a partir do binário (Fase 2+3) ----
if ($Regen) {
    if ($DiscPath -eq "") {
        Write-Host "❌ Use -DiscPath 'C:\caminho\para\RE3.cue' com -Regen" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "🔄 Executando recompilador PSXRecomp..." -ForegroundColor Cyan
    
    # O recompilador usa o game.toml + seeds para gerar o C
    & python "$ProjectRoot\psxrecomp\tools\recompile.py" `
        --config "$ProjectRoot\game.toml" `
        --disc   "$DiscPath" `
        --seeds  "$ProjectRoot\seeds\functions.csv" `
        --out    "$GeneratedDir"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Falha na recompilação. Verifique o game.toml e os seeds." -ForegroundColor Red
        exit 1
    }
    Write-Host "✅ Código C gerado em: $GeneratedDir" -ForegroundColor Green
}

# ---- Configurar CMake ----
Write-Host "⚙️  Configurando CMake..." -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

cmake -S "$ProjectRoot" `
      -B "$BuildDir" `
      -G Ninja `
      -DCMAKE_BUILD_TYPE=$Config `
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake configure falhou." -ForegroundColor Red
    exit 1
}

# ---- Build ----
Write-Host "🔨 Compilando..." -ForegroundColor Cyan
cmake --build "$BuildDir" --config $Config -- -j (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build falhou." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✅ Build concluído!" -ForegroundColor Green
Write-Host "   Executável: $BuildDir\Resident_Evil_3__Nemesis_Recompiled.exe" -ForegroundColor Green
Write-Host ""
Write-Host "Para executar:" -ForegroundColor Cyan
Write-Host "  .\build\Resident_Evil_3__Nemesis_Recompiled.exe" -ForegroundColor White
Write-Host ""
Write-Host "Atalhos de teclado:" -ForegroundColor Cyan
Write-Host "  Alt+Enter  → Alternar fullscreen/janela" -ForegroundColor White
Write-Host "  F11        → Fullscreen" -ForegroundColor White
