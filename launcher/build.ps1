# ============================================================
# Build script for Resident Evil 3 Launcher (MinGW / GCC)
# ============================================================
param()

$ScriptDir = $PSScriptRoot
# Build reproduzível: data do commit no lugar da hora do build, e nenhum
# timestamp no cabeçalho PE (mesmo commit + mesmo compilador = mesmo .exe).
$commitTime = & git -C $ScriptDir log -1 --format=%ct 2>$null
if ($LASTEXITCODE -eq 0 -and $commitTime) { $env:SOURCE_DATE_EPOCH = $commitTime.Trim() }
Write-Host "Compilando recursos do Launcher..." -ForegroundColor Cyan
windres "$ScriptDir\resource.rc" -O coff -o "$ScriptDir\resource.o"

Write-Host "Compilando executavel estatico do Launcher..." -ForegroundColor Cyan
g++ -O2 -static -mwindows -municode "$ScriptDir\main.cpp" "$ScriptDir\resource.o" -o "$ScriptDir\Resident_Evil_3_Launcher.exe" -lgdiplus -lcomctl32 -lcomdlg32 -lshlwapi -luxtheme "-Wl,--no-insert-timestamp"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Launcher compilado com sucesso: $ScriptDir\Resident_Evil_3_Launcher.exe" -ForegroundColor Green
    $ParentDir = Split-Path $ScriptDir -Parent
    $WorkspaceRoot = Split-Path $ParentDir -Parent

    # Se estivermos dentro de RE3Recomp\launcher, $WorkspaceRoot é a pasta raiz Decomp - SOTC
    # Se estivermos em Decomp - SOTC\launcher, $ParentDir é a pasta raiz Decomp - SOTC
    $candidatePCPorts = @(
        (Join-Path $ParentDir "ResidentEvil3-PCPort"),
        (Join-Path $WorkspaceRoot "ResidentEvil3-PCPort")
    )
    foreach ($PCPortDir in $candidatePCPorts) {
        if ($PCPortDir -and (Test-Path $PCPortDir)) {
            Copy-Item "$ScriptDir\Resident_Evil_3_Launcher.exe" "$PCPortDir\Resident_Evil_3_Launcher.exe" -Force
            Write-Host "Copiado para: $PCPortDir\Resident_Evil_3_Launcher.exe" -ForegroundColor Green
        }
    }
} else {
    Write-Host "Falha na compilacao do Launcher." -ForegroundColor Red
    exit 1
}
