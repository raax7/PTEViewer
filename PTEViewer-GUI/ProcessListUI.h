#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <imgui.h>

class ProcessListUI
{
public:
    ProcessListUI() : m_LastSearchBuffer(""), m_SearchBuffer(""), m_FirstListUpdate(true) {}
    ~ProcessListUI() = default;

private:
    struct ProcessListEntry
    {
        bool Running;
        std::chrono::steady_clock::time_point CloseAnimEndTime;     // Used to persist closed processes for a short period of time with a custom color.
        std::chrono::steady_clock::time_point CreateAnimEndTime;    // Used to indicate if a process is new with a custom color.

        uint64_t CR3;
        HANDLE ProcessId;
        HANDLE ParentProcessId;
        std::wstring ProcessName;

        std::vector<std::shared_ptr<ProcessListEntry>> Children;    // Used to build tree view.
    };

    enum ColumnIndex
    {
        ProcessNameIndex = 0,
        ProcessIdIndex,
        CR3Index,
        ColumnCount
    };

public:
    void Draw();

private:
    void DrawProcessEntry(const std::shared_ptr<ProcessListEntry>& Process, std::chrono::steady_clock::time_point Now);

    static bool SortByCR3(const std::shared_ptr<ProcessListEntry>& a, const std::shared_ptr<ProcessListEntry>& b, bool Ascending);
    static bool SortByProcessId(const std::shared_ptr<ProcessListEntry>& a, const std::shared_ptr<ProcessListEntry>& b, bool Ascending);
    static bool SortByProcessName(const std::shared_ptr<ProcessListEntry>& a, const std::shared_ptr<ProcessListEntry>& b, bool Ascending);

private:
    bool ShouldUpdateProcessList();
    bool ShouldRefilterProcessList();

    void RemoveClosedProcesses(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList, std::chrono::steady_clock::time_point Now);
    void UpdateProcessList(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList);
    void FilterProcessList(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList, std::vector<std::shared_ptr<ProcessListEntry>>& OutFilteredList, bool FirstIt = true, bool IsChild = false);
    void SortProcessList(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList, bool ForceResort, ImGuiTableSortSpecs* SortSpecs);

private:
    static constexpr int SEARCH_BUFFER_SIZE = 128;
    static constexpr ImColor NEW_COLOR = ImColor(0.f, 1.f, 0.f, 1.f);
    static constexpr ImColor TERMINATED_COLOR = ImColor(1.f, 0.f, 0.f, 1.f);
    static constexpr const char* CR3_FORMAT_STR = "0x%016I64X";
    static constexpr const char* CR3_INVALID_VALUE = "None";

private:
    std::vector<std::shared_ptr<ProcessListEntry>> m_ProcessList;
    std::vector<std::shared_ptr<ProcessListEntry>> m_FilteredProcessList;

    char m_LastSearchBuffer[SEARCH_BUFFER_SIZE];
    char m_SearchBuffer[SEARCH_BUFFER_SIZE];

    bool m_FirstListUpdate;
    bool m_Searching;
};
