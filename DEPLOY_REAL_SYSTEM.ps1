# ═══════════════════════════════════════════════════════════════════════════════
#                    CHIMERAMETALS - CLEAN BUILD & DEPLOY
#                           REAL FIX SYSTEM ONLY
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "              CHIMERAMETALS - REAL FIX TRADING SYSTEM" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# Stop processes
Write-Host "[1/5] Stopping processes..." -ForegroundColor Yellow
Stop-Process -Name "ChimeraMetal" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "ChimeraTelemetry" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Write-Host "    ✅ Done" -ForegroundColor Green
Write-Host ""

# Pull latest
Write-Host "[2/5] Pulling latest code..." -ForegroundColor Yellow
cd C:\ChimeraMetals
git reset --hard HEAD
git clean -fd
git pull origin main
Write-Host "    ✅ Code updated" -ForegroundColor Green
Write-Host ""

# Build ChimeraTelemetry
Write-Host "[3/5] Building ChimeraTelemetry WebSocket server..." -ForegroundColor Yellow
cd ChimeraTelemetry
Remove-Item -Path "build" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "build" -Force | Out-Null
cd build
cmake .. | Out-Null
cmake --build . --config Release | Out-Null
if (Test-Path "Release\ChimeraTelemetry.exe") {
    Write-Host "    ✅ ChimeraTelemetry.exe built" -ForegroundColor Green
} else {
    Write-Host "    ❌ Build failed!" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Build ChimeraMetal (REAL FIX SYSTEM)
Write-Host "[4/5] Building ChimeraMetal (REAL FIX system - 783 lines)..." -ForegroundColor Yellow
cd C:\ChimeraMetals
Remove-Item -Path "build" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "build" -Force | Out-Null
cd build
cmake .. | Out-Null
cmake --build . --config Release | Out-Null
if (Test-Path "Release\ChimeraMetal.exe") {
    Write-Host "    ✅ ChimeraMetal.exe built" -ForegroundColor Green
} else {
    Write-Host "    ❌ Build failed!" -ForegroundColor Red
    Write-Host "    Check if OpenSSL is installed at: C:\Program Files\OpenSSL-Win64" -ForegroundColor Yellow
    exit 1
}
Write-Host ""

# Start system
Write-Host "[5/5] Starting ChimeraMetal..." -ForegroundColor Yellow
cd Release
Start-Process -FilePath ".\ChimeraMetal.exe" -WindowStyle Normal
Start-Sleep -Seconds 3

$metal = Get-Process -Name "ChimeraMetal" -ErrorAction SilentlyContinue
$telemetry = Get-Process -Name "ChimeraTelemetry" -ErrorAction SilentlyContinue

if ($metal) {
    Write-Host "    ✅ ChimeraMetal.exe running (PID: $($metal.Id))" -ForegroundColor Green
}
if ($telemetry) {
    Write-Host "    ✅ ChimeraTelemetry.exe running (PID: $($telemetry.Id))" -ForegroundColor Green
}
Write-Host ""

# Done
Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host "                    ✅ DEPLOYMENT COMPLETE!" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host ""
Write-Host "📊 WHAT'S RUNNING:" -ForegroundColor Cyan
Write-Host "   • ChimeraMetal.exe    - REAL FIX 4.4 Trading System (783 lines)" -ForegroundColor White
Write-Host "   • ChimeraTelemetry.exe - WebSocket Dashboard Server" -ForegroundColor White
Write-Host ""
Write-Host "🌐 DASHBOARD: http://localhost:8080" -ForegroundColor Cyan
Write-Host ""
Write-Host "⚙️  CONFIG FILE: C:\ChimeraMetals\config.ini" -ForegroundColor Cyan
Write-Host "   Make sure it has your BlackBull FIX credentials!" -ForegroundColor Yellow
Write-Host ""
Write-Host "📋 WHAT THIS SYSTEM DOES:" -ForegroundColor Cyan
Write-Host "   • Connects to BlackBull FIX API via SSL/TLS" -ForegroundColor White
Write-Host "   • Authenticates with your credentials" -ForegroundColor White
Write-Host "   • Subscribes to XAUUSD market data" -ForegroundColor White
Write-Host "   • Streams real prices to dashboard" -ForegroundColor White
Write-Host "   • Handles heartbeats automatically" -ForegroundColor White
Write-Host ""
Write-Host "🔍 CHECK CONSOLE WINDOW for FIX connection status" -ForegroundColor Yellow
Write-Host ""

Start-Process "http://localhost:8080"
