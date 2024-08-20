#pragma once
#include <string>
#include <Windows.h>
#include <memory>
#include "../IA32.h"
#include "Driver.h"
#include "PageTableCache.h"
#include "SharedTypes.h"
#include "PageUtils.h"

#define PAGE_SHIFT 12L

constexpr uint8_t DirtyBit = 6;
constexpr uint8_t LargePageBit = 7;
constexpr uint8_t GlobalBit = 8;
constexpr uint8_t Sw_PrototypeBit = 9;
constexpr uint8_t Sw_TransitionBt = 10;

constexpr std::string NextElementCopy = ", ";

class PTViewerUI
{
public:
    PTViewerUI(HANDLE ProcessId, const std::wstring& ProcessName, uint64_t ProcessCR3)
        : m_ProcessId(ProcessId), m_ProcessName(ProcessName), m_ProcessCR3(ProcessCR3), m_Cache(std::make_unique<PageTableCache>(ProcessCR3))
    {
    }
    ~PTViewerUI() = default;

private:
    enum class ColumnIndex : int
    {
        // Extra/Custom.
        PageOverview = 0,
        PhysicalPageAddress,

        // Common page bits.
        Valid,
        Write,
        Supervisor,
        WriteThrough,
        CacheDisabled,
        Accessed,
        Dirty,
        LargePage,
        Global,
        PageFrameNumber,
        NoExecute,

        // Software-specific bits.
        Sw_Prototype,
        Sw_Transition,

        ColumnCount
    };
    enum class CopyState
    {
        None,

        CopyPageInfo,
        CopyAddress
    };

public:
    void Draw();

private:
    // Drawing.
    //
    void DrawEntry(const PML4E_64& Entry, PageUtils::PTIndexes& Index, PTEntryType PTType);
    void DrawTableColumns(const PageTableCache::PTData& Data, bool CopyPagingInfo, std::stringstream& CopyBuffer);
    bool DrawEntryTreeNode(const std::string& Name, bool SetupNextPTEntries, PTEntryType PTType, PageUtils::PTIndexes& Index);
    CopyState DrawEntryPopup(const PageUtils::AddressRange& Range, const PageTableCache::PTData& Data, PTEntryType PTType, std::stringstream& CopyBuffer);
    template <typename T> void DrawColumn(ColumnIndex Index, const T& Value, bool CopyingInfo, std::stringstream& CopyBuffer, const std::string& CopyLabel);

    // Handling and processing.
    //
    void HandleCopying(std::stringstream& CopyBuffer, CopyState State);
    void HandleNextEntry(const PageTableCache::PTData& PTData, PTEntryType PTType, PageUtils::PTIndexes& Index);
    template <typename T> void ProcessNextEntries(const T* Entries, PTEntryType NextPTType, PageUtils::PTIndexes& Index, uint16_t& NewIndex);

    // Misc.
    //
    void SetupTableColumns();
    PageTableCache::PTData GetPTData(const PML4E_64& Entry, const PageUtils::PTIndexes& Index, const PageUtils::AddressRange& Range, PTEntryType PTType);

public:
    const std::wstring& GetProcessName() { return m_ProcessName; }

private:
    HANDLE m_ProcessId;
    std::wstring m_ProcessName;
    uint64_t m_ProcessCR3;

    std::unique_ptr<PageTableCache> m_Cache = nullptr;
};
