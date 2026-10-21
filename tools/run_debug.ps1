# ============================================================
# Project RE: Visited - RE3: sessão de teste com ferramentas de debug
#
#   .\tools\run_debug.ps1            atualiza o exe de debug, abre o jogo e o capturador
#   .\tools\run_debug.ps1 -Build     recompila o build_dbg antes
#   .\tools\run_debug.ps1 -NoCapture só abre o jogo de debug (servidor TCP na porta 4370)
#
# O build de release (build/, o que vai no zip) não tem servidor de debug.
# Este script usa o build_dbg (Release + PSX_DEBUG_TOOLS=ON) e o copia para a
# pasta de execução com outro nome, para não se confundir com o exe de release.
# As capturas vão para <workspace>\captures, fora do repositório e do pacote.
# ============================================================
param(
    [switch]$Build,
    [switch]$NoCapture,
    [int]$Frames = 4
)

$ErrorActionPreference = "Stop"
# O atalho usa o Windows PowerShell 5.1. Aberto a partir de um PowerShell 7,
# ele herda o PSModulePath do 7 e perde os cmdlets básicos (Write-Host,
# Get-FileHash...). Restaura o caminho padrão do 5.1.
if ($PSVersionTable.PSVersion.Major -lt 6) {
    $env:PSModulePath = @(
        (Join-Path ([Environment]::GetFolderPath("MyDocuments")) "WindowsPowerShell\Modules"),
        (Join-Path $env:ProgramFiles "WindowsPowerShell\Modules"),
        (Join-Path $env:WINDIR "System32\WindowsPowerShell\v1.0\Modules")
    ) -join ";"
}
$Repo     = Split-Path $PSScriptRoot -Parent
$Workspace = Split-Path $Repo -Parent
$PCPort    = Join-Path $Workspace "ResidentEvil3-PCPort"
$DbgBuild  = Join-Path $Repo "build_dbg"
$DbgSrc    = Join-Path $DbgBuild "Resident_Evil_3__Nemesis_Recompiled.exe"
$DbgExe    = Join-Path $PCPort "Resident_Evil_3__Nemesis_Recompiled_debug.exe"

if ($Build) {
    if (-not (Get-Command git -ErrorAction SilentlyContinue) -and
        (Test-Path "$env:LOCALAPPDATA\Programs\GitPortable\cmd")) {
        $env:PATH = "$env:LOCALAPPDATA\Programs\GitPortable\cmd;$env:PATH"
    }
    $ct = git -C $Repo log -1 --format=%ct 2>$null
    if ($ct) { $env:SOURCE_DATE_EPOCH = $ct.Trim() }
    # PSX_DEBUG_TOOLS é option() em cache: passe sempre explícito.
    cmake -S $Repo -B $DbgBuild -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSX_DEBUG_TOOLS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    if ($LASTEXITCODE -ne 0) { throw "CMake configure do build_dbg falhou." }
    cmake --build $DbgBuild --config Release
    if ($LASTEXITCODE -ne 0) { throw "Build do build_dbg falhou." }
}

$cache = Select-String -Path (Join-Path $DbgBuild "CMakeCache.txt") -Pattern '^PSX_DEBUG_TOOLS:BOOL=ON' -Quiet
if (-not $cache) { throw "build_dbg não está com PSX_DEBUG_TOOLS=ON. Rode com -Build." }
if (-not (Test-Path $DbgSrc)) { throw "Exe de debug não encontrado em $DbgSrc. Rode com -Build." }

# Avisa se o exe de debug é mais velho que o de release (código desatualizado).
$rel = Join-Path $Repo "build\Resident_Evil_3__Nemesis_Recompiled.exe"
if ((Test-Path $rel) -and ((Get-Item $rel).LastWriteTime -gt (Get-Item $DbgSrc).LastWriteTime)) {
    Write-Host "[!] O build de release é mais novo que o build_dbg. Considere rodar com -Build." -ForegroundColor Yellow
}

if (-not (Test-Path $DbgExe) -or (Get-FileHash $DbgSrc).Hash -ne (Get-FileHash $DbgExe).Hash) {
    Copy-Item $DbgSrc $DbgExe -Force
    Write-Host "[OK] Exe de debug atualizado: $DbgExe" -ForegroundColor Green
}

if (Get-Process -Name "Resident_Evil_3__Nemesis_Recompiled*" -ErrorAction SilentlyContinue) {
    throw "Já existe uma instância do jogo aberta. Feche-a antes (a porta 4370 é única)."
}

# Mesma linha de comando que o launcher usa, com a BIOS do bios.cfg.
$bios = "bios\openbios.bin"
$biosCfg = Join-Path $PCPort "bios.cfg"
if (Test-Path $biosCfg) { $bios = (Get-Content $biosCfg -TotalCount 1).Trim() -replace '/', '\' }
Start-Process -FilePath $DbgExe -ArgumentList @("--bios", "`"$bios`"") -WorkingDirectory $PCPort
Write-Host "[OK] Jogo de debug iniciado (servidor TCP em 127.0.0.1:4370)." -ForegroundColor Green

if ($NoCapture) { return }

$py = "python"
$pyArgs = @()
if (Get-Command py -ErrorAction SilentlyContinue) { $py = "py"; $pyArgs += "-3" }
& $py @pyArgs (Join-Path $PSScriptRoot "ws_playtest.py") --frames $Frames --game-dir $PCPort
