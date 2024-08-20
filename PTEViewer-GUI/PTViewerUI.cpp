#include "PTViewerUI.h"
#include "../IA32.h"
#include "Driver.h"
#include "Globals.h"
#include "PageTableCache.h"
#include <imgui.h>
#include <sstream>

void PTViewerUI::Draw()
{
    if (ImGui::BeginTable("Page Tables", static_cast<int>(ColumnIndex::ColumnCount),
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollY))
    {
        // Ensure the column header row is always visible.
        ImGui::TableSetupScrollFreeze(0, 1);

        SetupTableColumns();
        ImGui::TableHeadersRow();

        PML4E_64* CachedPML4 = m_Cache->GetPML4();
        for (int i = 0; i < PML4E_ENTRY_COUNT_64; i++)
        {
            PML4E_64& PML4E = CachedPML4[i];
            if (PML4E.Flags == 0)
                continue;

            PageUtils::PTIndexes Index = { 0 };
            Index.PML4Index = i;

            DrawEntry(PML4E, Index, PTEntryType::PML4);
        }

        ImGui::EndTable();
    }
}
void PTViewerUI::DrawEntry(const PML4E_64& Entry, PageUtils::PTIndexes& Index, PTEntryType PTType)
{
    CopyState CopyState = CopyState::None;
    std::stringstream CopyBuffer;

    PageUtils::AddressRange PTRange = IndexToAddressRange(Index, PTType);
    PageTableCache::PTData PTData = GetPTData(Entry, Index, PTRange, PTType);

    bool TreeOpen = DrawEntryTreeNode(PTData.StringPageName, PTData.IsLeaf, PTType, Index);
    if (TreeOpen)
    {
        CopyState = DrawEntryPopup(PTRange, PTData, PTType, CopyBuffer);

        // Copy page name.
        if (CopyState == CopyState::CopyPageInfo)
        {
            CopyBuffer << PTData.StringPageName << NextElementCopy;
        }
    }

    // If we are copying page info, all open columns info will be pushed to the copy buffer.
    DrawTableColumns(PTData, CopyState == CopyState::CopyPageInfo, CopyBuffer);
    HandleCopying(CopyBuffer, CopyState);

    // Handle child entries if the tree node is open.
    if (TreeOpen)
    {
        HandleNextEntry(PTData, PTType, Index);
        ImGui::TreePop();
    }
}
void PTViewerUI::DrawTableColumns(const PageTableCache::PTData& Data, bool CopyPagingInfo, std::stringstream& CopyBuffer)
{
    DrawColumn(ColumnIndex::PhysicalPageAddress, Data.StringPhysicalAddress, CopyPagingInfo, CopyBuffer, "PhysicalAddress");
    DrawColumn(ColumnIndex::Valid, (bool)Data.Valid, CopyPagingInfo, CopyBuffer, "Valid");
    DrawColumn(ColumnIndex::Write, (bool)Data.Write, CopyPagingInfo, CopyBuffer, "Write");
    DrawColumn(ColumnIndex::Supervisor, (bool)Data.Supervisor, CopyPagingInfo, CopyBuffer, "Supervisor");
    DrawColumn(ColumnIndex::WriteThrough, (bool)Data.WriteThrough, CopyPagingInfo, CopyBuffer, "WriteThrough");
    DrawColumn(ColumnIndex::CacheDisabled, (bool)Data.CacheDisabled, CopyPagingInfo, CopyBuffer, "CacheDisabled");
    DrawColumn(ColumnIndex::Accessed, (bool)Data.Accessed, CopyPagingInfo, CopyBuffer, "Accessed");
    DrawColumn(ColumnIndex::Dirty, (bool)Data.Dirty, CopyPagingInfo, CopyBuffer, "Dirty");
    DrawColumn(ColumnIndex::LargePage, (bool)Data.LargePage, CopyPagingInfo, CopyBuffer, "LargePage");
    DrawColumn(ColumnIndex::Global, (bool)Data.Global, CopyPagingInfo, CopyBuffer, "Global");
    DrawColumn(ColumnIndex::PageFrameNumber, Data.StringPFN, CopyPagingInfo, CopyBuffer, "PFN");
    DrawColumn(ColumnIndex::NoExecute, (bool)Data.ExecuteDisable, CopyPagingInfo, CopyBuffer, "ExecuteDisable");
    DrawColumn(ColumnIndex::Sw_Prototype, (bool)Data.Sw_Prototype, CopyPagingInfo, CopyBuffer, "Sw_Prototype");
    DrawColumn(ColumnIndex::Sw_Transition, (bool)Data.Sw_Transition, CopyPagingInfo, CopyBuffer, "Sw_Transition");
}
bool PTViewerUI::DrawEntryTreeNode(const std::string& Name, bool IsLeaf, PTEntryType PTType, PageUtils::PTIndexes& Index)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    return ImGui::TreeNodeEx(Name.c_str(), IsLeaf ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None);
}
PTViewerUI::CopyState PTViewerUI::DrawEntryPopup(const PageUtils::AddressRange& Range, const PageTableCache::PTData& Data, PTEntryType PTType, std::stringstream& CopyBuffer)
{
    CopyState Result = CopyState::None;

    bool CanMakeMemoryResident = true;
    if (!Data.IsLeaf)
        CanMakeMemoryResident = false;
    else if (PTType < PTEntryType::PDPT)
        CanMakeMemoryResident = false;
    else if (PTType < PTEntryType::PT && !Data.LargePage)
        CanMakeMemoryResident = false;

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::Button("Copy"))
        {
            Result = CopyState::CopyPageInfo;
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy address"))
        {
            Result = CopyState::CopyAddress;
            CopyBuffer << "0x" << std::uppercase << std::hex << Range.StartAddress;
        }

        ImGui::BeginDisabled(!CanMakeMemoryResident);
        if (ImGui::Button("Make memory resident"))
        {
            Globals::GetDriver()->MakeMemoryResident(Range.StartAddress, m_ProcessId);
        }
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }

    return Result;
}
template <typename T> void PTViewerUI::DrawColumn(ColumnIndex Index, const T& Value, bool CopyingInfo, std::stringstream& CopyBuffer, const std::string& CopyLabel)
{
    if (ImGui::TableSetColumnIndex(static_cast<int>(Index)))
    {
        std::string TextString;

        if constexpr (std::is_same_v<T, bool>)
        {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, Value ? ImColor(0.f, 1.f, 0.f, 1.f) : ImColor(1.f, 0.f, 0.f, 1.f));
            TextString = PageUtils::BoolToString(Value);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            TextString = Value;
        }

        ImGui::Text(TextString.c_str());

        if (CopyingInfo)
        {
            CopyBuffer << CopyLabel << ": " << TextString << NextElementCopy;
        }
    }
}

void PTViewerUI::SetupTableColumns()
{
    ImGui::TableSetupColumn("Address Range", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoReorder);
    ImGui::TableSetupColumn("Physical Page Address", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Valid");
    ImGui::TableSetupColumn("Write", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Supervisor", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Write-Through", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Cache-Disabled", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Accessed", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Dirty", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Large-Page", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Global", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("PFN");
    ImGui::TableSetupColumn("No-Execute", ImGuiTableColumnFlags_DefaultHide);
    ImGui::TableSetupColumn("Prototype (SW)");
    ImGui::TableSetupColumn("Transition (SW)");
}
PageTableCache::PTData PTViewerUI::GetPTData(const PML4E_64& Entry, const PageUtils::PTIndexes& Index, const PageUtils::AddressRange& Range, PTEntryType PTType)
{
    PageTableCache::EntryKey Key = std::make_tuple(Index.PML4Index, Index.PDPTIndex, Index.PDIndex, Index.PTIndex, PTType);
    PageTableCache::PTData Data;
    if (!m_Cache->TryGetEntry(Key, Data))
    {
        Data = PageTableCache::PTData(Entry, Index, Range, PTType, Data.NextEntries);
        m_Cache->CacheEntry(Key, Data);
    }

    return Data;
}

void PTViewerUI::HandleCopying(std::stringstream& CopyBuffer, CopyState State)
{
    if (State != CopyState::None)
    {
        std::string CopyString = CopyBuffer.str();

        // If we are copying the full page info, trim the last NextElementCopy.
        if (State == CopyState::CopyPageInfo && CopyString.length() >= NextElementCopy.length())
        {
            CopyString = CopyString.substr(0, CopyString.length() - NextElementCopy.length());
        }

        ImGui::SetClipboardText(CopyString.c_str());
    }
}
void PTViewerUI::HandleNextEntry(const PageTableCache::PTData& PTData, PTEntryType PTType, PageUtils::PTIndexes& Index)
{
    if (PTData.IsLeaf)
        return;

    switch (PTType)
    {
    case PTEntryType::PML4:
        ProcessNextEntries(PTData.NextEntries.PML4, PTEntryType::PDPT, Index, Index.PDPTIndex);
        break;
    case PTEntryType::PDPT:
        ProcessNextEntries(PTData.NextEntries.PDPT, PTEntryType::PD, Index, Index.PDIndex);
        break;
    case PTEntryType::PD:
        ProcessNextEntries(PTData.NextEntries.PD, PTEntryType::PT, Index, Index.PTIndex);
        break;
    }
}
template <typename T> void PTViewerUI::ProcessNextEntries(const T* Entries, PTEntryType NextPTType, PageUtils::PTIndexes& Index, uint16_t& NewIndex)
{
    for (int i = 0; i < PML4E_ENTRY_COUNT_64; i++)
    {
        const T& NextEntry = Entries[i];
        if (NextEntry.Flags)
        {
            NewIndex = i;
            DrawEntry((PML4E_64&)NextEntry, Index, NextPTType);
        }
    }
}
