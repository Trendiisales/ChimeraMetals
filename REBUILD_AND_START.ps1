# ==============================================================================
#                   REBUILD AND RESTART WITH VERBOSE LOGGING
# ==============================================================================

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "              REBUILD WITH VERBOSE FIX LOGGING" -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""

# Stop processes
Write-Host "[1/3] Stopping chimera..." -ForegroundColor Yellow
Stop-Process -Name "chimera" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "ChimeraTelemetry" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Write-Host "    [OK] Stopped" -ForegroundColor Green
Write-Host ""

# Pull and rebuild
Write-Host "[2/3] Pulling latest code and rebuilding..." -ForegroundColor Yellow
cd C:\ChimeraMetals
git pull origin main

cd BASELINE_20260223_035615
Remove-Item -Path "build" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "build" -Force | Out-Null
cd build
cmake .. 2>&1 | Out-Null
cmake --build . --config Release 2>&1 | Out-Null

if (Test-Path "Release\chimera.exe") {
    Write-Host "    [OK] chimera.exe rebuilt" -ForegroundColor Green
} else {
    Write-Host "    [ERROR] Build failed!" -ForegroundColor Red
    exit 1
}

# Copy config.ini
Copy-Item "C:\ChimeraMetals\BASELINE_20260223_035615\config.ini" "Release\config.ini" -Force
Write-Host "    [OK] config.ini copied" -ForegroundColor Green
Write-Host ""

# Start
Write-Host "[3/3] Starting chimera.exe..." -ForegroundColor Yellow
Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "            YOU SHOULD NOW SEE THIS SEQUENCE:" -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  [FIX] Attempting connection to demo-uk-eqx-02.p.c-trader.com:5211..." -ForegroundColor White
Write-Host "  [FIX] SSL CONNECTED" -ForegroundColor White
Write-Host "  [FIX] LOGON SENT" -ForegroundColor White
Write-Host "  [FIX] LOGON ACCEPTED" -ForegroundColor White
Write-Host "  [FIX] SECURITY LIST REQUEST SENT" -ForegroundColor White
Write-Host "  [FIX] SECURITY LIST RECEIVED" -ForegroundColor White
Write-Host "  [FIX] MARKET DATA REQUEST SENT FOR XAUUSD/XAGUSD" -ForegroundColor White
Write-Host "  === MARKET DATA ===" -ForegroundColor White
Write-Host "  XAUUSD: 2650.50 / 2650.56" -ForegroundColor White
Write-Host "  XAGUSD: 22.45 / 22.48" -ForegroundColor White
Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "If you DON'T see these messages, the FIX connection is failing!" -ForegroundColor Yellow
Write-Host ""

cd Release
.\chimera.exe
