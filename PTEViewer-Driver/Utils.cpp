#include "Utils.h"
#include <intrin.h>

namespace Utils
{
    namespace
    {
        ULONG_PTR InvalidateTLB(
            _In_ ULONG_PTR Argument)
        {
            UNREFERENCED_PARAMETER(Argument);

            __writecr3(__readcr3());
            __wbinvd();

            return 0;
        }
    }

    NTSTATUS ReadPhysicalMemory(
        _In_ UINT64 TargetAddress,
        _Inout_ PVOID OutBuffer,
        _In_ SIZE_T BufferSize)
    {
        MM_COPY_ADDRESS SourceAddress = { 0 };
        SourceAddress.PhysicalAddress.QuadPart = TargetAddress;
        return MmCopyMemory(OutBuffer, SourceAddress, BufferSize, MM_COPY_MEMORY_PHYSICAL, &BufferSize);
    }

    VOID InvalidateAllCaches()
    {
        KeInvalidateAllCaches();
        KeIpiGenericCall(InvalidateTLB, NULL);
    }
}