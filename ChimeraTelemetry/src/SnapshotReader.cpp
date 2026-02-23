#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "TelemetrySnapshot.hpp"

static TelemetrySnapshot* g_snapshot = nullptr;

bool SnapshotInit()
{
    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "Global\\ChimeraTelemetrySharedMemory");
    if (!hMap) return false;

    g_snapshot = (TelemetrySnapshot*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(TelemetrySnapshot));
    return g_snapshot != nullptr;
}

bool SnapshotRead(TelemetrySnapshot& out)
{
    if (!g_snapshot) return false;

    uint64_t seq1;
    uint64_t seq2;

    do
    {
        seq1 = g_snapshot->sequence.load(std::memory_order_acquire);
        std::memcpy(&out, g_snapshot, sizeof(TelemetrySnapshot));
        seq2 = g_snapshot->sequence.load(std::memory_order_acquire);

    } while (seq1 != seq2);

    return true;
}
