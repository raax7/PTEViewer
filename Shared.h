#pragma once
#ifdef _KERNEL_MODE
#include <ntifs.h>
#else
#include <Windows.h>
#endif
#include "IA32.h"

#define _DEVICENAME L"\\Device\\PTEViewer"
#define _SYMBOLICLINKNAME L"\\DosDevices\\PTEViewer"
#define _DEVICENAME2 "\\\\.\\PTEViewer"

#define IOCTL_QUERYCR3 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUERYPAGETABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PAGEINMEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INVALIDATEALLCACHES CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(push, 1)
typedef struct _QUERYCR3
{
    _In_ HANDLE ProcessId;
    _Out_ UINT64 CR3;
} QUERYCR3, * PQUERYCR3;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _QUERYPAGETABLE
{
    _In_ UINT64 Address;
    union
    {
        _Out_ PML4E_64 PML4[PML4E_ENTRY_COUNT_64];
        _Out_ PDPTE_64 PDPT[PDPTE_ENTRY_COUNT_64];
        _Out_ PDE_64 PD[PDE_ENTRY_COUNT_64];
        _Out_ PTE_64 PT[PTE_ENTRY_COUNT_64];
    };
} QUERYPAGETABLE, * PQUERYPAGETABLE;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _PAGEINMEMORY
{
    _In_ HANDLE ProcessId;
    _In_ PVOID Address;
} PAGEINMEMORY, * PPAGEINMEMORY;
#pragma pack(pop)