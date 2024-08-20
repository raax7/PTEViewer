#pragma once
#include <map>
#include <Windows.h>
#include "PollEntry.h"

class ProcessManager
{
public:
    ProcessManager();
    ~ProcessManager();

public:
    struct ProcessInfo
    {
        uint64_t CR3;
        HANDLE ProcessId;
        HANDLE ParentProcessId;
        std::wstring szExeFile;
    };

public:
    const std::shared_ptr<PollEntry>& GetPollEntry() const { return m_PollEntry; }
    const std::vector<ProcessInfo>& GetRunningProcesses() const { return m_RunningProcesses; }

private:
    void PollCallback();

private:
    std::shared_ptr<PollEntry> m_PollEntry;
    std::vector<ProcessInfo> m_RunningProcesses;
};
