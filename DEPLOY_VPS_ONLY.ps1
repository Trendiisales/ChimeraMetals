# ==============================================================================
#        CHIMERAMETALS BASELINE - VPS DEPLOYMENT (NO BROWSER LAUNCH)
#                   Dashboard opens on Mac Chrome separately
# ==============================================================================

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "       CHIMERAMETALS BASELINE - VPS DEPLOYMENT" -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""

#[1/6] Stop processes
Write-Host "[1/6] Stopping processes..." -ForegroundColor Yellow
Stop-Process -Name "chimera" -Force -ErrorAction SilentlyContinue
Stop-Process -Name "ChimeraTelemetry" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Write-Host "    [OK] Done" -ForegroundColor Green
Write-Host ""

# [2/6] Pull latest
Write-Host "[2/6] Pulling latest code..." -ForegroundColor Yellow
cd C:\ChimeraMetals
git reset --hard HEAD
git clean -fd
git pull origin main
Write-Host "    [OK] Code updated" -ForegroundColor Green
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
    Write-Host "    [OK] ChimeraTelemetry.exe built" -ForegroundColor Green
} else {
    Write-Host "    [ERROR] Build failed!" -ForegroundColor Red
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
    Write-Host "    [OK] chimera.exe built (BASELINE system)" -ForegroundColor Green
} else {
    Write-Host "    [ERROR] Build failed!" -ForegroundColor Red
    Write-Host "    Check if OpenSSL is installed" -ForegroundColor Yellow
    exit 1
}
Write-Host ""

# [5/6] Check config
Write-Host "[5/6] Checking configuration..." -ForegroundColor Yellow
if (Test-Path "C:\ChimeraMetals\BASELINE_20260223_035615\config.ini") {
    Write-Host "    [OK] config.ini found" -ForegroundColor Green
} else {
    Write-Host "    [WARN] config.ini NOT FOUND - creating template" -ForegroundColor Yellow
    
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
    Write-Host "    [WARN] EDIT config.ini with your BlackBull credentials!" -ForegroundColor Yellow
}
Write-Host ""

# [6/6] Start system
Write-Host "[6/6] Starting BASELINE ChimeraMetals..." -ForegroundColor Yellow
Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "                  WATCH FOR FIX LOGON SEQUENCE BELOW:" -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "You should see:" -ForegroundColor Yellow
Write-Host "  [FIX] SSL CONNECTED" -ForegroundColor White
Write-Host "  [FIX] LOGON SENT" -ForegroundColor White
Write-Host "  [FIX] LOGON ACCEPTED" -ForegroundColor White
Write-Host "  [FIX] SECURITY LIST RECEIVED" -ForegroundColor White
Write-Host "  === MARKET DATA ===" -ForegroundColor White
Write-Host "  XAUUSD: 2650.50 / 2650.56" -ForegroundColor White
Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""

cd C:\ChimeraMetals\BASELINE_20260223_035615\build\Release

# Get VPS IP address
$ipAddress = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.IPAddress -like "185.*"}).IPAddress
if (-not $ipAddress) {
    $ipAddress = "185.167.119.59"
}

Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Green
Write-Host "                    VPS DEPLOYMENT COMPLETE!" -ForegroundColor Green
Write-Host "=======================================================================" -ForegroundColor Green
Write-Host ""
Write-Host "VPS IS RUNNING:" -ForegroundColor Cyan
Write-Host "   chimera.exe          : BASELINE Trading System (FIX 4.4)" -ForegroundColor White
Write-Host "   ChimeraTelemetry.exe : WebSocket Server on :8080" -ForegroundColor White
Write-Host ""
Write-Host "TO VIEW DASHBOARD ON YOUR MAC:" -ForegroundColor Yellow
Write-Host ""
Write-Host "   Open Chrome and go to:" -ForegroundColor White
Write-Host "   http://$ipAddress:8080" -ForegroundColor Cyan
Write-Host ""
Write-Host "   The dashboard will connect to this VPS automatically!" -ForegroundColor Green
Write-Host ""
Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Starting chimera.exe NOW - watch for FIX logon sequence..." -ForegroundColor Yellow
Write-Host ""

# Start chimera.exe in THIS console (not hidden) so you can see FIX logon
.\chimera.exe
