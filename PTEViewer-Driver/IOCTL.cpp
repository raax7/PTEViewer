#include "IOCTL.h"
#include "Globals.h"
#include "Utils.h"

#include <intrin.h>

#include "../IA32.h"
#include "../Shared.h"

namespace IOCTL
{
    namespace
    {
        PDEVICE_OBJECT g_DeviceObject = nullptr;
        UNICODE_STRING g_DeviceName = { 0 }, g_SymbolicLinkName = { 0 };

        NTSTATUS ControlIO(
            _In_ PDEVICE_OBJECT DeviceObject,
            _In_ PIRP Irp)
        {
            UNREFERENCED_PARAMETER(DeviceObject);

            NTSTATUS Status = STATUS_SUCCESS;

            PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
            ULONG ControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
            ULONG OutputBufferLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
            PVOID SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

            if (!SystemBuffer)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            switch (ControlCode)
            {
            case IOCTL_QUERYCR3:
            {
                if (OutputBufferLength != sizeof(QUERYCR3))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                PQUERYCR3 Buffer = static_cast<PQUERYCR3>(SystemBuffer);
                PEPROCESS Process = nullptr;
                HANDLE ProcessId = Buffer->ProcessId;

                Status = PsLookupProcessByProcessId(ProcessId, &Process);
                if (!NT_SUCCESS(Status))
                {
                    DBG("Failed to find process for querying CR3! (0x%lX)", Status);
                    break;
                }

                KeAttachProcess(Process);
                Buffer->CR3 = __readcr3();
                KeDetachProcess();

                ObDereferenceObject(Process);

                Irp->IoStatus.Information = sizeof(QUERYCR3);
                break;
            }
            case IOCTL_QUERYPAGETABLE:
            {
                if (OutputBufferLength != sizeof(QUERYPAGETABLE))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                constexpr SIZE_T RequiredSize = sizeof(PML4E_64) * PML4E_ENTRY_COUNT_64;
                PQUERYPAGETABLE Buffer = static_cast<PQUERYPAGETABLE>(SystemBuffer);

                Status = Utils::ReadPhysicalMemory(Buffer->Address, &Buffer->PML4, RequiredSize);
                if (!NT_SUCCESS(Status))
                {
                    DBG("Failed to read physical memory for page tables at address 0x%p! (0x%lX)", Buffer->Address, Status);
                    break;
                }

                Irp->IoStatus.Information = sizeof(QUERYPAGETABLE);
                break;
            }
            case IOCTL_PAGEINMEMORY:
            {
                if (OutputBufferLength != sizeof(PAGEINMEMORY))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                PPAGEINMEMORY Buffer = static_cast<PPAGEINMEMORY>(SystemBuffer);
                PEPROCESS Process = nullptr;
                PMDL Mdl = nullptr;

                Status = PsLookupProcessByProcessId(Buffer->ProcessId, &Process);
                if (!NT_SUCCESS(Status))
                {
                    DBG("Failed to find process for paging-in memory! (0x%lX)", Status);
                    break;
                }

                KeAttachProcess(Process);
                Mdl = IoAllocateMdl(Buffer->Address, PAGE_SIZE, false, false, nullptr);
                if (Mdl)
                {
                    __try
                    {
                        MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
                        MmUnlockPages(Mdl);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        DBG("Failed to lock pages for paging-in memory!");
                    }

                    IoFreeMdl(Mdl);
                }
                else
                {
                    DBG("Failed to allocate MDL for paging-in memory!");
                }
                KeDetachProcess();

                ObDereferenceObject(Process);
                break;
            }
            case IOCTL_INVALIDATEALLCACHES:
            {
                Utils::InvalidateAllCaches();

                Irp->IoStatus.Information = 0;
                break;
            }

            default:
                Status = STATUS_NOT_SUPPORTED;
                break;
            }

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return Status;
        }
        NTSTATUS UnsupportedIO(
            _In_ PDEVICE_OBJECT DeviceObject,
            _In_ PIRP Irp)
        {
            UNREFERENCED_PARAMETER(DeviceObject);

            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Irp->IoStatus.Status;
        }
        NTSTATUS CreateIO(
            _In_ PDEVICE_OBJECT DeviceObject,
            _In_ PIRP Irp)
        {
            UNREFERENCED_PARAMETER(DeviceObject);

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Irp->IoStatus.Status;
        }
        NTSTATUS CloseIO(
            _In_ PDEVICE_OBJECT DeviceObject,
            _In_ PIRP Irp)
        {
            UNREFERENCED_PARAMETER(DeviceObject);

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Irp->IoStatus.Status;
        }
    }

    NTSTATUS Setup(_In_ PDRIVER_OBJECT DriverObject)
    {
        NTSTATUS Status = STATUS_SUCCESS;

        RtlInitUnicodeString(&g_DeviceName, _DEVICENAME);
        RtlInitUnicodeString(&g_SymbolicLinkName, _SYMBOLICLINKNAME);

        Status = IoCreateDevice(DriverObject, 0, &g_DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &g_DeviceObject);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        Status = IoCreateSymbolicLink(&g_SymbolicLinkName, &g_DeviceName);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        SetFlag(g_DeviceObject->Flags, DO_BUFFERED_IO);
        for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            DriverObject->MajorFunction[i] = IOCTL::UnsupportedIO;
        }

        DriverObject->MajorFunction[IRP_MJ_CREATE] = IOCTL::CreateIO;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = IOCTL::CloseIO;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IOCTL::ControlIO;
        ClearFlag(g_DeviceObject->Flags, DO_DEVICE_INITIALIZING);

        return STATUS_SUCCESS;
    }
    void Destroy()
    {
        if (g_SymbolicLinkName.Buffer)
            IoDeleteSymbolicLink(&g_SymbolicLinkName);

        if (g_DeviceObject)
            IoDeleteDevice(g_DeviceObject);
    }
}