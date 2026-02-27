# ChimeraMetals v2.1 - Complete Production System

## Version: 2.1.0-FINAL
## Date: 2025-02-27
## Status: Production Ready with Full Diagnostics

### What's Included

**Core System:**
- main.cpp - Complete trading engine with diagnostics
- TelemetryWriter.hpp - Expanded telemetry (30+ metrics)
- config.ini - Trading mode configuration

**Dashboard:**
- TelemetryServer.cpp - HTTP server with expanded JSON API
- index.html - Professional glassmorphism dashboard
- chimera_logo.png - Branding

### Features

✅ Trading Modes (SAFE/SHADOW/LIVE)
✅ Institutional latency tracking (execution + network RTT)
✅ Active ping every 5 seconds for continuous latency
✅ p50/p95 percentiles with spike detection
✅ Latency hard-halt at 80ms
✅ Complete diagnostic logging
✅ Zero-price bug fix
✅ Position tracking ready
✅ FIX session monitoring

### Deployment

```powershell
# Extract to C:\ChimeraMetals
Copy-Item src\* C:\ChimeraMetals\src\ -Recurse -Force
Copy-Item config.ini C:\ChimeraMetals\config.ini

# Build
cd C:\ChimeraMetals\build
cmake --build . --config Release

# Run
cd Release
.\ChimeraMetals.exe
```

### Configuration

Edit config.ini:
```ini
[mode]
mode = SHADOW  # SAFE, SHADOW, or LIVE
```

### Dashboard Access

http://YOUR_VPS_IP:7777

### Diagnostic Output

Console shows:
- [STATE] - Trading conditions
- [HFT] - HFT engine signals
- [STRUCTURE] - Structure engine signals
- [ENGINE] - Execution triggers
- [SHADOW/LIVE] - Trade execution
- [BLOCKED] - What's blocking trades

### Architecture

**8 Safety Layers:**
1. Trading mode (SAFE/SHADOW/LIVE)
2. Compliance audit
3. Stale tick guard
4. Rate limiter (20/min)
5. Latency halt (>80ms)
6. HFT entry filter
7. Risk governor
8. ProductionCore approval

### License
Proprietary - ChimeraMetals Engineering
