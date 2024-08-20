#include <ntifs.h>
#include "Globals.h"
#include "IOCTL.h"

extern "C" {
    VOID DriverUnload(
        _In_ PDRIVER_OBJECT DriverObject)
    {
        UNREFERENCED_PARAMETER(DriverObject);

        DBG("Unloading driver...");
        IOCTL::Destroy();
        DBG("Unloaded driver!");
    }

    NTSTATUS DriverEntry(
        _In_ PDRIVER_OBJECT DriverObject,
        _In_ PUNICODE_STRING RegistryPath)
    {
        UNREFERENCED_PARAMETER(RegistryPath);
        DBG("Loading driver...");

        DriverObject->DriverUnload = DriverUnload;

        NTSTATUS Status = IOCTL::Setup(DriverObject);
        if (!NT_SUCCESS(Status))
        {
            DBG("Failed to setup IOCTL! (0x%lX)", Status);
            return Status;
        }

        DBG("Driver loaded!");
        return STATUS_SUCCESS;
    }
}