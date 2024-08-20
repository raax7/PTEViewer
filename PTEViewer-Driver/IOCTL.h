#pragma once
#include <ntifs.h>

namespace IOCTL
{
    NTSTATUS Setup(_In_ PDRIVER_OBJECT DriverObject);
    void Destroy();
}