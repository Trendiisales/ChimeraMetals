#include "TelemetrySnapshot.hpp"
#include <windows.h>
#include <iostream>

static HANDLE g_map = nullptr;
static TelemetrySnapshot* g_snap = nullptr;

bool SnapshotInit()
{
    g_map = OpenFileMappingA(
        FILE_MAP_READ,
        FALSE,
        "ChimeraTelemetrySharedMemory"
    );

    if (!g_map)
        return false;

    g_snap = (TelemetrySnapshot*)MapViewOfFile(
        g_map,
        FILE_MAP_READ,
        0,
        0,
        sizeof(TelemetrySnapshot)
    );

    return g_snap != nullptr;
}

bool SnapshotRead(TelemetrySnapshot& out)
{
    uint64_t seq1 = g_snap->sequence.load(std::memory_order_acquire);

    memcpy(&out, g_snap, sizeof(TelemetrySnapshot));

    uint64_t seq2 = g_snap->sequence.load(std::memory_order_acquire);

    return seq1 == seq2;
}
