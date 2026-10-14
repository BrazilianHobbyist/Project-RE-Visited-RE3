# ============================================================
# Build script for Resident Evil 3 Launcher (MinGW / GCC)
# ============================================================
param()

$ScriptDir = $PSScriptRoot
Write-Host "Compilando recursos do Launcher..." -ForegroundColor Cyan
windres "$ScriptDir\resource.rc" -O coff -o "$ScriptDir\resource.o"

Write-Host "Compilando executavel estatico do Launcher..." -ForegroundColor Cyan
g++ -O2 -static -mwindows -municode "$ScriptDir\main.cpp" "$ScriptDir\resource.o" -o "$ScriptDir\Resident_Evil_3_Launcher.exe" -lgdiplus -lcomctl32 -lcomdlg32 -lshlwapi -luxtheme

if ($LASTEXITCODE -eq 0) {
    Write-Host "Launcher compilado com sucesso: $ScriptDir\Resident_Evil_3_Launcher.exe" -ForegroundColor Green
    $PCPortDir = Join-Path (Split-Path $ScriptDir -Parent) "ResidentEvil3-PCPort"
    if (Test-Path $PCPortDir) {
        Copy-Item "$ScriptDir\Resident_Evil_3_Launcher.exe" "$PCPortDir\Resident_Evil_3_Launcher.exe" -Force
        Write-Host "Copiado para: $PCPortDir\Resident_Evil_3_Launcher.exe" -ForegroundColor Green
    }
} else {
    Write-Host "Falha na compilacao do Launcher." -ForegroundColor Red
}
