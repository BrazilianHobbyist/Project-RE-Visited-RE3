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

    $RootLauncherDir = Join-Path $WorkspaceRoot "launcher"
    if ($RootLauncherDir -and (Test-Path $RootLauncherDir) -and ($RootLauncherDir -ne $ScriptDir)) {
        Copy-Item "$ScriptDir\main.cpp" "$RootLauncherDir\main.cpp" -Force
        Copy-Item "$ScriptDir\Resident_Evil_3_Launcher.exe" "$RootLauncherDir\Resident_Evil_3_Launcher.exe" -Force
        Write-Host "Sincronizado com: $RootLauncherDir" -ForegroundColor Green
    }
} else {
    Write-Host "Falha na compilacao do Launcher." -ForegroundColor Red
    exit 1
}
