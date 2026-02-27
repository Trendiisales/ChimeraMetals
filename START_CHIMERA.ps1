# Quick Fix: Copy config.ini and run chimera.exe

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "                    CONFIG FIX + START SYSTEM" -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "[1/2] Copying config.ini to Release folder..." -ForegroundColor Yellow

$source = "C:\ChimeraMetals\BASELINE_20260223_035615\config.ini"
$dest = "C:\ChimeraMetals\BASELINE_20260223_035615\build\Release\config.ini"

if (Test-Path $source) {
    Copy-Item $source $dest -Force
    Write-Host "    [OK] config.ini copied" -ForegroundColor Green
    Write-Host "    From: $source" -ForegroundColor Gray
    Write-Host "    To:   $dest" -ForegroundColor Gray
} else {
    Write-Host "    [ERROR] Source config.ini not found!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[2/2] Starting chimera.exe..." -ForegroundColor Yellow
Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "           WATCH FOR FIX LOGON SEQUENCE BELOW:" -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""

cd C:\ChimeraMetals\BASELINE_20260223_035615\build\Release
.\chimera.exe
