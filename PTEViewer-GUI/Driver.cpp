#include "Driver.h"

Driver::Driver()
{
    m_Device = CreateFileA("\\\\.\\PTEViewer", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_Device == INVALID_HANDLE_VALUE)
        MessageBoxA(NULL, "Failed to open handle to device!", "", MB_ICONERROR);
}
Driver::~Driver()
{
    CloseHandle(m_Device);
}

bool Driver::QueryCR3(HANDLE ProcessId, QUERYCR3* Out)
{
    ZeroMemory(Out, sizeof(*Out));
    Out->ProcessId = ProcessId;

    return DeviceIoControl(m_Device, IOCTL_QUERYCR3, Out, sizeof(*Out), Out, sizeof(*Out), nullptr, nullptr);
}
bool Driver::QueryPageTable(uint64_t PTAddress, QUERYPAGETABLE* Out)
{
    ZeroMemory(Out, sizeof(*Out));
    Out->Address = PTAddress;

    return DeviceIoControl(m_Device, IOCTL_QUERYPAGETABLE, Out, sizeof(*Out), Out, sizeof(*Out), nullptr, nullptr);
}
bool Driver::MakeMemoryResident(uint64_t Address, HANDLE ProcessId)
{
    PAGEINMEMORY Buffer = { 0 };
    Buffer.Address = reinterpret_cast<PVOID>(Address);
    Buffer.ProcessId = ProcessId;

    return DeviceIoControl(m_Device, IOCTL_PAGEINMEMORY, &Buffer, sizeof(Buffer), &Buffer, sizeof(Buffer), nullptr, nullptr);
}
