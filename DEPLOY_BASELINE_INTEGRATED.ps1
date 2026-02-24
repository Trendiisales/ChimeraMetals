# ═══════════════════════════════════════════════════════════════════════════════
#            CHIMERAMETALS BASELINE + TELEMETRY - COMPLETE DEPLOYMENT
#                   Auto-Launch Dashboard | Shared Memory IPC
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "       CHIMERAMETALS BASELINE + TELEMETRY DEPLOYMENT" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

#[1/6] Stop processes
Write-Host "[1/6] Stopping processes..." -ForegroundColor Yellow
Stop-Process -Name "chimera" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "ChimeraTelemetry" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Write-Host "    ✅ Done" -ForegroundColor Green
Write-Host ""

# [2/6] Pull latest
Write-Host "[2/6] Pulling latest code..." -ForegroundColor Yellow
cd C:\ChimeraMetals
git reset --hard HEAD
git clean -fd
git pull origin main
Write-Host "    ✅ Code updated" -ForegroundColor Green
Write-Host ""

# [3/6] Build ChimeraTelemetry
Write-Host "[3/6] Building ChimeraTelemetry WebSocket server..." -ForegroundColor Yellow
cd ChimeraTelemetry
Remove-Item -Path "build" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "build" -Force | Out-Null
cd build
cmake .. 2>&1 | Out-Null
cmake --build . --config Release 2>&1 | Out-Null
if (Test-Path "Release\ChimeraTelemetry.exe") {
    Write-Host "    ✅ ChimeraTelemetry.exe built" -ForegroundColor Green
} else {
    Write-Host "    ❌ Build failed!" -ForegroundColor Red
    exit 1
}
Write-Host ""

# [4/6] Build BASELINE ChimeraMetals
Write-Host "[4/6] Building BASELINE ChimeraMetals system..." -ForegroundColor Yellow
cd C:\ChimeraMetals\BASELINE_20260223_035615
Remove-Item -Path "build" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "build" -Force | Out-Null
cd build
cmake .. 2>&1 | Out-Null
cmake --build . --config Release 2>&1 | Out-Null
if (Test-Path "Release\chimera.exe") {
    Write-Host "    ✅ chimera.exe built (BASELINE system)" -ForegroundColor Green
} else {
    Write-Host "    ❌ Build failed!" -ForegroundColor Red
    Write-Host "    Check if OpenSSL is installed at: C:\Program Files\OpenSSL-Win64" -ForegroundColor Yellow
    exit 1
}
Write-Host ""

# [5/6] Check config
Write-Host "[5/6] Checking configuration..." -ForegroundColor Yellow
if (Test-Path "C:\ChimeraMetals\BASELINE_20260223_035615\config.ini") {
    Write-Host "    ✅ config.ini found" -ForegroundColor Green
} else {
    Write-Host "    ⚠️  config.ini NOT FOUND!" -ForegroundColor Yellow
    Write-Host "    Creating template config.ini..." -ForegroundColor Cyan
    
    $configContent = @"
[fix]
host=fix-api.blackbull.com
port=443
sender_comp_id=YOUR_CLIENT_ID
target_comp_id=BLACKBULL
username=YOUR_API_KEY
password=YOUR_API_SECRET
heartbeat_interval=30
"@
    $configContent | Out-File -FilePath "C:\ChimeraMetals\BASELINE_20260223_035615\config.ini" -Encoding ASCII
    Write-Host "    ⚠️  EDIT config.ini with your BlackBull credentials!" -ForegroundColor Yellow
}
Write-Host ""

# [6/6] Start system
Write-Host "[6/6] Starting BASELINE ChimeraMetals..." -ForegroundColor Yellow
cd C:\ChimeraMetals\BASELINE_20260223_035615\build\Release
Start-Process -FilePath ".\chimera.exe" -WindowStyle Normal
Start-Sleep -Seconds 5

$chimera = Get-Process -Name "chimera" -ErrorAction SilentlyContinue
$telemetry = Get-Process -Name "ChimeraTelemetry" -ErrorAction SilentlyContinue

if ($chimera) {
    Write-Host "    ✅ chimera.exe running (PID: $($chimera.Id))" -ForegroundColor Green
}
if ($telemetry) {
    Write-Host "    ✅ ChimeraTelemetry.exe running (PID: $($telemetry.Id))" -ForegroundColor Green
} else {
    Write-Host "    ⚠️  ChimeraTelemetry not detected (may start shortly)" -ForegroundColor Yellow
}
Write-Host ""

# Done
Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host "                    ✅ DEPLOYMENT COMPLETE!" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host ""
Write-Host "📊 WHAT'S RUNNING:" -ForegroundColor Cyan
Write-Host "   • chimera.exe          - BASELINE Trading System (FIX 4.4)" -ForegroundColor White
Write-Host "   • ChimeraTelemetry.exe - WebSocket Dashboard Server" -ForegroundColor White
Write-Host ""
Write-Host "🌐 DASHBOARD: http://localhost:8080" -ForegroundColor Cyan
Write-Host ""
Write-Host "📈 REAL-TIME DATA FLOW:" -ForegroundColor Cyan
Write-Host "   chimera.exe → Shared Memory → ChimeraTelemetry.exe → Browser" -ForegroundColor White
Write-Host ""
Write-Host "🔧 SYSTEM FEATURES:" -ForegroundColor Cyan
Write-Host "   ✓ Full FIX 4.4 protocol" -ForegroundColor White
Write-Host "   ✓ SSL/TLS encryption" -ForegroundColor White
Write-Host "   ✓ Real BlackBull market data (XAUUSD, XAGUSD)" -ForegroundColor White
Write-Host "   ✓ Auto-launched dashboard" -ForegroundColor White
Write-Host "   ✓ Glassmorphic UI" -ForegroundColor White
Write-Host "   ✓ 10Hz telemetry updates" -ForegroundColor White
Write-Host ""
Write-Host "⚙️  CONFIG: C:\ChimeraMetals\BASELINE_20260223_035615\config.ini" -ForegroundColor Cyan
Write-Host "📁 BINARY: C:\ChimeraMetals\BASELINE_20260223_035615\build\Release\chimera.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "🔍 CHECK CONSOLE WINDOW for FIX connection status" -ForegroundColor Yellow
Write-Host ""

Start-Sleep -Seconds 2
Start-Process "http://localhost:8080"

Write-Host "Dashboard opening in browser..." -ForegroundColor Cyan
Write-Host ""
