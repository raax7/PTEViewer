#include "ProcessListUI.h"
#include <algorithm>

#include "Globals.h"

#include "ProcessManager.h"
#include "PTViewerUI.h"
#include "Config.h"

void ProcessListUI::Draw()
{
    bool ShouldUpdateList = ShouldUpdateProcessList();
    bool ShouldRefilterList = ShouldRefilterProcessList();
    bool ForceResort = false;

    auto Now = std::chrono::steady_clock::now();

    m_Searching = m_SearchBuffer[0] != '\0';
    if (ShouldUpdateList)
    {
        ForceResort = true;
        UpdateProcessList(m_ProcessList);
    }
    if (ShouldUpdateList || ShouldRefilterList)
    {
        ForceResort = true;
        FilterProcessList(m_ProcessList, m_FilteredProcessList);
    }

    ImGui::Checkbox("Raw-Order", &Config::cfg_NoProcessSorting);
    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
    {
        Globals::GetProcessManager()->GetPollEntry()->ForcePoll();
    }
    ImGui::SameLine();
    ImGui::InputText("Search", m_SearchBuffer, IM_ARRAYSIZE(m_SearchBuffer));

    if (ImGui::BeginTable("ProcessTable", ColumnCount,
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        (Config::cfg_NoProcessSorting ? ImGuiTableFlags_None : ImGuiTableFlags_Sortable) |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollY))
    {
        // Ensure the column header row is always visible.
        //
        ImGui::TableSetupScrollFreeze(0, 1);

        // The order of these functions affects sorting.
        // If you need to change the order, also change it in HandleSorting.
        //
        ImGui::TableSetupColumn("Process Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoReorder);
        ImGui::TableSetupColumn("Process ID", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("CR3", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableHeadersRow();

        SortProcessList(m_FilteredProcessList, ForceResort, ImGui::TableGetSortSpecs());

        for (auto& Process : m_FilteredProcessList)
        {
            DrawProcessEntry(Process, Now);
        }

        ImGui::EndTable();
    }

    // Remove closed processes if their animation (color change) is done.
    //
    RemoveClosedProcesses(m_ProcessList, Now);
}
void ProcessListUI::DrawProcessEntry(const std::shared_ptr<ProcessListEntry>& Process, std::chrono::steady_clock::time_point Now)
{
    ImGui::TableNextRow();

    // Set color based on state.
    //
    if (!Process->Running)
    {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, TERMINATED_COLOR);
    }
    else
    {
        auto TimeRemaining = std::chrono::duration_cast<std::chrono::milliseconds>(Now - Process->CreateAnimEndTime).count();
        if (TimeRemaining < 0)
        {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, NEW_COLOR);
        }
    }

    bool UseLeaf = false;
    bool TreeOpen = false;
    if (ImGui::TableSetColumnIndex(ProcessNameIndex))
    {
        UseLeaf = Process->Children.empty();
        TreeOpen = ImGui::TreeNodeEx((void*)(intptr_t)Process->ProcessId,
            (UseLeaf ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None) |
            (m_Searching ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen) |
            ImGuiTreeNodeFlags_SpanAllColumns,
            "%ls", Process->ProcessName.c_str());

        if (ImGui::BeginPopupContextItem("Inspect Process"))
        {
            if (ImGui::Button("View process PTs"))
                Globals::g_InspectedPTs.push_back(std::make_unique<PTViewerUI>(Process->ProcessId, Process->ProcessName, Process->CR3));

            ImGui::EndPopup();
        }
    }

    if (ImGui::TableSetColumnIndex(ProcessIdIndex))
        ImGui::Text("%d", Process->ProcessId);

    if (ImGui::TableSetColumnIndex(CR3Index))
    {
        if (Process->CR3) { ImGui::Text(CR3_FORMAT_STR, Process->CR3); }
        else { ImGui::Text(CR3_INVALID_VALUE); }
    }

    if (TreeOpen)
    {
        // If we are searching we cannot display the children.
        //
        if (!m_Searching && !Process->Children.empty())
        {
            for (const auto& Child : Process->Children)
            {
                DrawProcessEntry(Child, Now);
            }
        }

        ImGui::TreePop();
    }
}

bool ProcessListUI::SortByCR3(const std::shared_ptr<ProcessListEntry>& a, const std::shared_ptr<ProcessListEntry>& b, bool Ascending)
{
    return Ascending ? a->CR3 < b->CR3 : a->CR3 > b->CR3;
}
bool ProcessListUI::SortByProcessId(const std::shared_ptr<ProcessListEntry>& a, const std::shared_ptr<ProcessListEntry>& b, bool Ascending)
{
    return Ascending ? a->ProcessId < b->ProcessId : a->ProcessId > b->ProcessId;
}
bool ProcessListUI::SortByProcessName(const std::shared_ptr<ProcessListEntry>& a, const std::shared_ptr<ProcessListEntry>& b, bool Ascending)
{
    int cmp = wcscmp(a->ProcessName.c_str(), b->ProcessName.c_str());
    return Ascending ? cmp < 0 : cmp > 0;
}

bool ProcessListUI::ShouldUpdateProcessList()
{
    return Globals::GetProcessManager()->GetPollEntry()->PollOccurred();
}
bool ProcessListUI::ShouldRefilterProcessList()
{
    return strcmp(m_LastSearchBuffer, m_SearchBuffer) != 0;
}

void ProcessListUI::RemoveClosedProcesses(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList, std::chrono::steady_clock::time_point Now)
{
    auto It = ProcessList.begin();
    while (It != ProcessList.end())
    {
        auto& Entry = *It;
        if (!Entry->Running)
        {
            auto TimeRemaining = std::chrono::duration_cast<std::chrono::milliseconds>(Now - Entry->CloseAnimEndTime).count();
            if (TimeRemaining >= 0)
            {
                It = ProcessList.erase(It);
                continue;
            }
        }

        // Check children as well.
        //
        if (!Entry->Children.empty())
            RemoveClosedProcesses(Entry->Children, Now);

        ++It;
    }
}
void ProcessListUI::UpdateProcessList(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList)
{
    const auto Now = std::chrono::steady_clock::now();

    const std::vector<ProcessManager::ProcessInfo>& ProcessMap = Globals::GetProcessManager()->GetRunningProcesses();
    std::unordered_map<HANDLE, std::shared_ptr<ProcessListEntry>> UpdatedProcessListMap;

    // Add new or update existing processes.
    //
    for (const auto& Process : ProcessMap)
    {
        if (Process.ProcessId == INVALID_HANDLE_VALUE || !Process.ProcessId)
            continue;

        auto NewEntry = std::make_shared<ProcessListEntry>();
        NewEntry->Running = true;

        // This is to prevent all processes from appearing with NEW_COLOR when you open the program.
        //
        if (m_FirstListUpdate)
            NewEntry->CreateAnimEndTime = std::chrono::steady_clock::time_point{};
        else
            NewEntry->CreateAnimEndTime = Now + std::chrono::milliseconds(Config::cfg_NewProcessStickTimeMs);

        NewEntry->CloseAnimEndTime = std::chrono::steady_clock::time_point{};

        NewEntry->CR3 = Process.CR3;
        NewEntry->ProcessId = Process.ProcessId;
        NewEntry->ParentProcessId = Process.ParentProcessId;
        NewEntry->ProcessName = std::wstring(Process.szExeFile);

        UpdatedProcessListMap[NewEntry->ProcessId] = NewEntry;
    }

    // Update/persist animation times.
    //
    for (auto& Entry : ProcessList)
    {
        std::function<void(const std::shared_ptr<ProcessListEntry>&)> UpdateProcessListEntry
            = [&](const std::shared_ptr<ProcessListEntry>& Entry)
            {
                if (Entry->ProcessId == INVALID_HANDLE_VALUE || Entry->ProcessId == 0)
                    return;

                const auto& It = UpdatedProcessListMap.find(Entry->ProcessId);
                if (It == UpdatedProcessListMap.end())
                {
                    // Process was not found in the new list, mark it as closed.
                    // Set CloseAnimEndTime if it hasn't been already.
                    //
                    if (Entry->Running)
                    {
                        Entry->Running = false; // Now this if will only be triggered once.
                        Entry->CloseAnimEndTime = Now + std::chrono::milliseconds(Config::cfg_ClosedProcessStickTimeMs);
                    }

                    UpdatedProcessListMap[Entry->ProcessId] = Entry;
                }
                else
                {
                    // Process was found, ensure FirstSeenTime is persisted.
                    //
                    It->second->CreateAnimEndTime = Entry->CreateAnimEndTime;
                }

                // Recursively update all children.
                //
                for (auto& Child : Entry->Children)
                {
                    UpdateProcessListEntry(Child);
                }
            };

        UpdateProcessListEntry(Entry);
    }

    // Link children to parents.
    //
    for (auto& [ProcessId, Entry] : UpdatedProcessListMap)
    {
        if (UpdatedProcessListMap.find(Entry->ParentProcessId) != UpdatedProcessListMap.end())
        {
            UpdatedProcessListMap[Entry->ParentProcessId]->Children.push_back(Entry);
        }
    }

    // Clear and refill with the updated values.
    //
    ProcessList.clear();
    for (const auto& [PID, Entry] : UpdatedProcessListMap)
    {
        if (Entry->ParentProcessId == 0 || UpdatedProcessListMap.find(Entry->ParentProcessId) == UpdatedProcessListMap.end())
        {
            ProcessList.push_back(Entry);
        }
    }

    m_FirstListUpdate = false;
}
void ProcessListUI::FilterProcessList(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList, std::vector<std::shared_ptr<ProcessListEntry>>& OutFilteredList, bool FirstIt, bool IsChild)
{
    if (FirstIt)
    {
        OutFilteredList.clear();

        // Update the last search term.
        //
        strcpy_s(m_LastSearchBuffer, SEARCH_BUFFER_SIZE, m_SearchBuffer);

        // If there is no search buffer, we can skip.
        //
        if (m_SearchBuffer[0] == '\0')
        {
            OutFilteredList = ProcessList;
            return;
        }
    }

    std::string Search = m_SearchBuffer;
    std::transform(Search.begin(), Search.end(), Search.begin(), ::tolower);

    std::wstring WideSearch = std::wstring(Search.begin(), Search.end());

    for (const auto& It : ProcessList)
    {
        std::wstring ProcessName = It->ProcessName;
        std::transform(ProcessName.begin(), ProcessName.end(), ProcessName.begin(), ::tolower);

        // Format CR3 in the same way it's displayed in the GUI.
        //
        char FormattedCR3[64] = "";
        sprintf_s(FormattedCR3, sizeof(FormattedCR3), CR3_FORMAT_STR, It->CR3);
        const std::string CR3Str(It->CR3 ? FormattedCR3 : CR3_INVALID_VALUE);

        if (ProcessName.find(WideSearch) != std::string::npos
            || std::to_string(reinterpret_cast<UINT64>(It->ProcessId)).find(Search) != std::string::npos
            || CR3Str.find(Search) != std::string::npos)
        {
            OutFilteredList.push_back(It);
        }

        FilterProcessList(It->Children, OutFilteredList, false, true);
    }
}
void ProcessListUI::SortProcessList(std::vector<std::shared_ptr<ProcessListEntry>>& ProcessList, bool ForceResort, ImGuiTableSortSpecs* SortSpecs)
{
    if (SortSpecs && (SortSpecs->SpecsDirty || ForceResort))
    {
        using SortFunction = bool(*)(const std::shared_ptr<ProcessListEntry>&, const std::shared_ptr<ProcessListEntry>&, bool);
        static const SortFunction SortFunctions[] = {
            SortByProcessName, // ProcessNameIndex.
            SortByProcessId,         // ProcessIdIndex.
            SortByCR3          // CR3Index.
        };

        bool Ascending = SortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
        int ColumnIndex = SortSpecs->Specs[0].ColumnIndex;

        if (ColumnIndex >= 0 && ColumnIndex < sizeof(SortFunctions) / sizeof(SortFunctions[0]))
        {
            SortFunction SortFunc = SortFunctions[ColumnIndex];

            std::function<void(std::vector<std::shared_ptr<ProcessListEntry>>&)> SortRecursively;
            SortRecursively = [&](std::vector<std::shared_ptr<ProcessListEntry>>& List) {
                std::sort(List.begin(), List.end(), [SortFunc, Ascending](const std::shared_ptr<ProcessListEntry>& a, const std::shared_ptr<ProcessListEntry>& b) {
                    return SortFunc(a, b, Ascending);
                    });

                // Recursively sort children for each process.
                //
                for (auto& Entry : List)
                {
                    if (Entry->Children.empty() == false)
                    {
                        SortRecursively(Entry->Children);
                    }
                }
                };

            SortRecursively(ProcessList);
        }

        SortSpecs->SpecsDirty = false;
    }
}
