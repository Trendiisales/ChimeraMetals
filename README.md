# ChimeraMetals v2.1 - Dashboard & Telemetry Expansion

## Latest Update: 2025-02-27

### ✅ Features Added
- Professional glassmorphism dashboard with real-time data  
- HTTP telemetry server on port 7777
- Expanded telemetry structure (30+ metrics)
- Zero-price bug fix with last-valid-price fallback
- Position tracking, performance metrics, FIX session monitoring

### 🚀 Quick Start
```powershell
cd C:\ChimeraMetals\build
cmake --build . --config Release
cd Release
.\ChimeraMetals.exe
```

Access dashboard: `http://YOUR_VPS_IP:7777`

### 📦 Files
- `src/main.cpp` - Main trading engine with telemetry
- `src/TelemetryWriter.hpp` - Expanded telemetry structure
- `src/gui/TelemetryServer.cpp` - HTTP server + API
- `src/gui/www/index.html` - Professional dashboard
- `DASHBOARD_EXPANSION.md` - Roadmap for metrics

### 🎯 Status
✅ Dashboard live and streaming  
✅ Market data flowing  
✅ Zero compilation warnings  
⏳ Latency calculation (pending)  
⏳ Performance metrics (pending)

Version: 2.1.0  
Author: ChimeraMetals Engineering  
License: Proprietary
