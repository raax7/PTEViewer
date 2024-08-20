#pragma once
#include <ntifs.h>

namespace Utils
{
    NTSTATUS ReadPhysicalMemory(
        _In_ UINT64 TargetAddress,
        _Inout_ PVOID OutBuffer,
        _In_ SIZE_T BufferSize);

    VOID InvalidateAllCaches();
}