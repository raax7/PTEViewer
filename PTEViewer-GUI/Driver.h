#pragma once
#include <Windows.h>
#include <cstdint>
#include "../Shared.h"

class Driver
{
public:
    Driver();
    ~Driver();

private:
    HANDLE m_Device;

public:
    bool QueryCR3(HANDLE ProcessId, QUERYCR3* Out);
    bool QueryPageTable(uint64_t PTAddress, QUERYPAGETABLE* Out);
    bool MakeMemoryResident(uint64_t Address, HANDLE ProcessId);
};
