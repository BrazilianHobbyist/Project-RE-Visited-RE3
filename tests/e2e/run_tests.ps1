param(
    [ValidateSet(1, 2, 3, 4, 5)]
    [int]$Tier,

    [switch]$VerboseOutput,

    [string]$Keyword
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..\..")

$RunnerScript = Join-Path $ScriptDir "run_e2e_tests.py"
$ArgList = @("$RunnerScript")

if ($PSBoundParameters.ContainsKey('Tier')) {
    $ArgList += "--tier"
    $ArgList += "$Tier"
}

if ($VerboseOutput) {
    $ArgList += "-v"
}

if ($Keyword) {
    $ArgList += "-k"
    $ArgList += "$Keyword"
}

Write-Host "Running RE3Recomp E2E Test Suite..." -ForegroundColor Cyan
Write-Host "Project Root: $ProjectRoot" -ForegroundColor Gray

& python $ArgList
exit $LASTEXITCODE
