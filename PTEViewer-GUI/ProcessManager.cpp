#include "ProcessManager.h"
#include "Globals.h"
#include "Config.h"
#include "Driver.h"
#include "PollManager.h"

#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

ProcessManager::ProcessManager()
{
    m_PollEntry = std::make_shared<PollEntry>(
        std::bind(&ProcessManager::PollCallback, this),
        Config::cfg_ProcessPollIntervalMs
    );
    Globals::GetPollManager()->AddPollEntry(m_PollEntry);
}
ProcessManager::~ProcessManager()
{
    if (m_PollEntry)
    {
        Globals::GetPollManager()->RemovePollEntry(m_PollEntry);
    }
}

void ProcessManager::PollCallback()
{
    m_RunningProcesses.clear();

    ULONG RequiredSize = 0;
    NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &RequiredSize);

    RequiredSize += 0x500;
    PSYSTEM_PROCESS_INFORMATION ProcessInformation = static_cast<PSYSTEM_PROCESS_INFORMATION>(malloc(RequiredSize));
    if (!ProcessInformation)
        return;

    if (!NT_SUCCESS(NtQuerySystemInformation(SystemProcessInformation, ProcessInformation, RequiredSize, nullptr)))
    {
        free(ProcessInformation);
        return;
    }

    PSYSTEM_PROCESS_INFORMATION CurrentProcess = ProcessInformation;
    do
    {
        ProcessInfo Info = { 0 };
        Info.szExeFile = std::wstring(CurrentProcess->ImageName.Buffer, CurrentProcess->ImageName.Length / sizeof(wchar_t));;
        Info.ProcessId = CurrentProcess->UniqueProcessId;
        Info.ParentProcessId = *reinterpret_cast<HANDLE*>((PUCHAR)(&CurrentProcess->UniqueProcessId) + sizeof(CurrentProcess->UniqueProcessId));

        QUERYCR3 QueriedCR3 = { 0 };
        if (Globals::GetDriver()->QueryCR3(Info.ProcessId, &QueriedCR3))
        {
            Info.CR3 = QueriedCR3.CR3;
        }
        else
        {
            Info.CR3 = 0;
        }

        m_RunningProcesses.push_back(Info);

        if (!CurrentProcess->NextEntryOffset)
            break;

        CurrentProcess = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)CurrentProcess + CurrentProcess->NextEntryOffset);
    } while (true);

    free(ProcessInformation);
}
