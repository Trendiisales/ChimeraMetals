#pragma once

// ChimeraMetals Version Control
// DO NOT MODIFY - Auto-generated version header

#define CHIMERA_VERSION_MAJOR 1
#define CHIMERA_VERSION_MINOR 0
#define CHIMERA_VERSION_PATCH 0
#define CHIMERA_BUILD_DATE "2025-02-26"
#define CHIMERA_BUILD_TIME "01:40:00 UTC"
#define CHIMERA_GIT_HASH "PRODUCTION_FINAL"

#define CHIMERA_VERSION_STRING "1.0.0-PRODUCTION_FINAL"
#define CHIMERA_FULL_VERSION "ChimeraMetals v1.0.0 (2025-02-26 01:40:00 UTC) PRODUCTION_FINAL"

// File version tracking - Updated on every file modification
#define FILE_VERSION_FIXSESSION      "1.0.0-20250226-0140"
#define FILE_VERSION_FIXGAPRECOVERY  "1.0.0-20250226-0140"
#define FILE_VERSION_WATCHDOG        "1.0.0-20250226-0140"
#define FILE_VERSION_PLATFORM        "1.0.0-20250226-0140"
#define FILE_VERSION_MAIN            "1.0.0-20250226-0140"
#define FILE_VERSION_CMAKE           "1.0.0-20250226-0140"

// Compilation requirements
#define REQUIRED_CPP_STANDARD 20
#define REQUIRED_OPENSSL_MIN "3.0.0"
#define REQUIRED_BOOST_MIN "1.75.0"

// Build fingerprint - Ensures cohesive package
#define BUILD_FINGERPRINT "CHIMERA_PROD_FINAL_20250226_0140_COHESIVE_v1.0.0"

namespace chimera {
    constexpr const char* getVersion() { return CHIMERA_VERSION_STRING; }
    constexpr const char* getFullVersion() { return CHIMERA_FULL_VERSION; }
    constexpr const char* getBuildFingerprint() { return BUILD_FINGERPRINT; }
}
