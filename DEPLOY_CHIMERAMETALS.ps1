# ═══════════════════════════════════════════════════════════════════════════════
#                    CHIMERAMETALS V3 - ONE-CLICK DEPLOYMENT
#                         PowerShell Drop-In Script
# ═══════════════════════════════════════════════════════════════════════════════

$ErrorActionPreference = "Stop"

Write-Host "═══════════════════════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "              CHIMERAMETALS V3 - AUTOMATED DEPLOYMENT SCRIPT" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION 1: PRE-FLIGHT CHECKS
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "[1/7] Running pre-flight checks..." -ForegroundColor Yellow

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "    ⚠️  WARNING: Not running as Administrator. Some operations may fail." -ForegroundColor Yellow
} else {
    Write-Host "    ✅ Running as Administrator" -ForegroundColor Green
}

# Check if Git is installed
$gitPath = (Get-Command git -ErrorAction SilentlyContinue)
if (-not $gitPath) {
    Write-Host "    ❌ Git not found in PATH" -ForegroundColor Red
    Write-Host "    Please install Git: https://git-scm.com/download/win" -ForegroundColor Red
    exit 1
} else {
    Write-Host "    ✅ Git found: $($gitPath.Path)" -ForegroundColor Green
}

# Check if CMake is installed
$cmakePath = (Get-Command cmake -ErrorAction SilentlyContinue)
if (-not $cmakePath) {
    Write-Host "    ❌ CMake not found in PATH" -ForegroundColor Red
    Write-Host "    Please install CMake: https://cmake.org/download/" -ForegroundColor Red
    exit 1
} else {
    Write-Host "    ✅ CMake found: $($cmakePath.Path)" -ForegroundColor Green
}

# Check for Visual Studio Build Tools
$msbuildPath = (Get-Command msbuild -ErrorAction SilentlyContinue)
if (-not $msbuildPath) {
    Write-Host "    ⚠️  MSBuild not found. Attempting to locate Visual Studio..." -ForegroundColor Yellow
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($vsPath) {
            $env:Path += ";$vsPath\MSBuild\Current\Bin"
            Write-Host "    ✅ Found Visual Studio at: $vsPath" -ForegroundColor Green
        }
    }
}

Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION 2: BACKUP EXISTING FILES
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "[2/7] Creating backup of existing files..." -ForegroundColor Yellow

$backupDir = "C:\ChimeraMetals\BACKUP_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
if (Test-Path "C:\ChimeraMetals") {
    New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
    Copy-Item -Path "C:\ChimeraMetals\*" -Destination $backupDir -Recurse -Force
    Write-Host "    ✅ Backup created at: $backupDir" -ForegroundColor Green
} else {
    Write-Host "    ℹ️  No existing installation found (fresh install)" -ForegroundColor Cyan
}

Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION 3: PULL LATEST CODE FROM GITHUB
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "[3/7] Pulling latest code from GitHub..." -ForegroundColor Yellow

if (Test-Path "C:\ChimeraMetals\.git") {
    Set-Location "C:\ChimeraMetals"
    Write-Host "    Repository exists, pulling latest changes..." -ForegroundColor Cyan
    git fetch origin
    git reset --hard origin/main
    Write-Host "    ✅ Code updated to latest version" -ForegroundColor Green
} else {
    Write-Host "    Repository not found, cloning..." -ForegroundColor Cyan
    if (Test-Path "C:\ChimeraMetals") {
        Remove-Item "C:\ChimeraMetals" -Recurse -Force
    }
    git clone https://github.com/Trendiisales/ChimeraMetals.git C:\ChimeraMetals
    Write-Host "    ✅ Repository cloned successfully" -ForegroundColor Green
}

Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION 4: STOP RUNNING PROCESSES
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "[4/7] Stopping running ChimeraMetals processes..." -ForegroundColor Yellow

$processes = @("ChimeraMetal", "ChimeraTelemetry")
foreach ($proc in $processes) {
    $running = Get-Process -Name $proc -ErrorAction SilentlyContinue
    if ($running) {
        Stop-Process -Name $proc -Force
        Write-Host "    ✅ Stopped: $proc.exe" -ForegroundColor Green
    } else {
        Write-Host "    ℹ️  Not running: $proc.exe" -ForegroundColor Cyan
    }
}

# Wait for processes to fully terminate
Start-Sleep -Seconds 2

Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION 5: REBUILD ChimeraTelemetry SERVER
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "[5/7] Building ChimeraTelemetry WebSocket Server..." -ForegroundColor Yellow

Set-Location "C:\ChimeraMetals\ChimeraTelemetry"

# Clean previous build
if (Test-Path "build") {
    Remove-Item "build" -Recurse -Force
    Write-Host "    Cleaned previous build directory" -ForegroundColor Cyan
}

# Create build directory
New-Item -ItemType Directory -Path "build" -Force | Out-Null
Set-Location "build"

# Run CMake
Write-Host "    Running CMake..." -ForegroundColor Cyan
cmake .. 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "    ❌ CMake configuration failed" -ForegroundColor Red
    exit 1
}

# Build Release configuration
Write-Host "    Building Release configuration..." -ForegroundColor Cyan
cmake --build . --config Release 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "    ❌ Build failed" -ForegroundColor Red
    exit 1
}

if (Test-Path "Release\ChimeraTelemetry.exe") {
    Write-Host "    ✅ ChimeraTelemetry.exe built successfully" -ForegroundColor Green
} else {
    Write-Host "    ❌ ChimeraTelemetry.exe not found after build" -ForegroundColor Red
    exit 1
}

Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION 6: REBUILD ChimeraMetal TRADING ENGINE
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "[6/7] Building ChimeraMetal Trading Engine..." -ForegroundColor Yellow

Set-Location "C:\ChimeraMetals"

# Clean previous build
if (Test-Path "build") {
    Remove-Item "build" -Recurse -Force
    Write-Host "    Cleaned previous build directory" -ForegroundColor Cyan
}

# Create build directory
New-Item -ItemType Directory -Path "build" -Force | Out-Null
Set-Location "build"

# Run CMake
Write-Host "    Running CMake..." -ForegroundColor Cyan
cmake .. 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "    ❌ CMake configuration failed" -ForegroundColor Red
    exit 1
}

# Build Release configuration
Write-Host "    Building Release configuration..." -ForegroundColor Cyan
cmake --build . --config Release 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "    ❌ Build failed" -ForegroundColor Red
    exit 1
}

if (Test-Path "Release\ChimeraMetal.exe") {
    Write-Host "    ✅ ChimeraMetal.exe built successfully" -ForegroundColor Green
} else {
    Write-Host "    ❌ ChimeraMetal.exe not found after build" -ForegroundColor Red
    exit 1
}

Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# SECTION 7: START SERVICES
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "[7/7] Starting ChimeraMetals services..." -ForegroundColor Yellow

# Start ChimeraMetal (which will auto-launch ChimeraTelemetry)
Set-Location "C:\ChimeraMetals\build\Release"

Write-Host "    Starting ChimeraMetal.exe..." -ForegroundColor Cyan
Start-Process -FilePath ".\ChimeraMetal.exe" -WindowStyle Normal

# Wait for services to initialize
Start-Sleep -Seconds 3

# Check if processes are running
$metalRunning = Get-Process -Name "ChimeraMetal" -ErrorAction SilentlyContinue
$telemetryRunning = Get-Process -Name "ChimeraTelemetry" -ErrorAction SilentlyContinue

if ($metalRunning) {
    Write-Host "    ✅ ChimeraMetal.exe is running (PID: $($metalRunning.Id))" -ForegroundColor Green
} else {
    Write-Host "    ❌ ChimeraMetal.exe failed to start" -ForegroundColor Red
}

if ($telemetryRunning) {
    Write-Host "    ✅ ChimeraTelemetry.exe is running (PID: $($telemetryRunning.Id))" -ForegroundColor Green
} else {
    Write-Host "    ⚠️  ChimeraTelemetry.exe not detected (may start shortly)" -ForegroundColor Yellow
}

Write-Host ""

# ═══════════════════════════════════════════════════════════════════════════════
# DEPLOYMENT COMPLETE
# ═══════════════════════════════════════════════════════════════════════════════

Write-Host "═══════════════════════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host "                    ✅ DEPLOYMENT COMPLETE!" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host ""
Write-Host "🌐 DASHBOARD ACCESS:" -ForegroundColor Cyan
Write-Host "   Local:    http://localhost:8080" -ForegroundColor White
Write-Host "   Network:  http://185.167.119.59:8080" -ForegroundColor White
Write-Host ""
Write-Host "📊 WHAT'S RUNNING:" -ForegroundColor Cyan
Write-Host "   • ChimeraMetal.exe    - Trading Engine" -ForegroundColor White
Write-Host "   • ChimeraTelemetry.exe - WebSocket Server" -ForegroundColor White
Write-Host ""
Write-Host "📂 FILE LOCATIONS:" -ForegroundColor Cyan
Write-Host "   • Binaries:  C:\ChimeraMetals\build\Release\" -ForegroundColor White
Write-Host "   • Dashboard: C:\ChimeraMetals\dashboard\" -ForegroundColor White
Write-Host "   • Backup:    $backupDir" -ForegroundColor White
Write-Host ""
Write-Host "🔍 VERIFY DEPLOYMENT:" -ForegroundColor Cyan
Write-Host "   1. Open http://localhost:8080 in browser" -ForegroundColor White
Write-Host "   2. Check status indicator shows 🟢 LIVE" -ForegroundColor White
Write-Host "   3. Verify data is updating (not all zeros)" -ForegroundColor White
Write-Host ""
Write-Host "🛠️  TROUBLESHOOTING:" -ForegroundColor Cyan
Write-Host "   • View logs: Check console window for errors" -ForegroundColor White
Write-Host "   • Restart:   Run this script again" -ForegroundColor White
Write-Host "   • Restore:   Copy files from backup directory" -ForegroundColor White
Write-Host ""
Write-Host "Press any key to open dashboard in browser..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

# Open dashboard in default browser
Start-Process "http://localhost:8080"

Write-Host ""
Write-Host "Deployment script complete. Happy trading! 🚀" -ForegroundColor Green
Write-Host ""
