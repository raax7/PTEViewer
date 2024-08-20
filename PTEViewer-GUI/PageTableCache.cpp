#include "Config.h"
#include "Globals.h"
#include "PageTableCache.h"
#include "PollEntry.h"
#include "PTViewerUI.h"
#include "PageUtils.h"

PageTableCache::PageTableCache(uint64_t ProcessCR3)
    : m_ProcessCR3(ProcessCR3), m_PML4()
{
    m_PollEntry = std::make_shared<PollEntry>(
        std::bind(&PageTableCache::PollCallback, this),
        Config::cfg_PageTablePollIntervalMs
    );
    Globals::GetPollManager()->AddPollEntry(m_PollEntry);
}
PageTableCache::~PageTableCache()
{
    if (m_PollEntry)
    {
        Globals::GetPollManager()->RemovePollEntry(m_PollEntry);
    }
}

void PageTableCache::CacheEntry(const EntryKey& Key, const PTData& PTData)
{
    m_Cache[Key] = PTData;
}
bool PageTableCache::TryGetEntry(const EntryKey& Key, PTData& PTData)
{
    auto it = m_Cache.find(Key);
    if (it != m_Cache.end())
    {
        PTData = it->second;
        return true;
    }
    return false;
}
void PageTableCache::Clear()
{
    m_Cache.clear();
}

void PageTableCache::PollCallback()
{
    // Clear all saved cache.
    Clear();

    // Reconstruct top level cache.
    QUERYPAGETABLE PML4 = { 0 };
    Globals::GetDriver()->QueryPageTable(m_ProcessCR3, &PML4);
    memcpy(m_PML4, PML4.PML4, sizeof(m_PML4));
}

PageTableCache::PTData::PTData(const PML4E_64& Entry, const PageUtils::PTIndexes& Index, const PageUtils::AddressRange& Range, PTEntryType Type, QUERYPAGETABLE& NextEntries)
{
    IsLeaf = PageUtils::PopulateNextEntries(Entry, Type, NextEntries);
    this->NextEntries = NextEntries;
    PhysicalAddress = Entry.PageFrameNumber << PAGE_SHIFT;

    // Common bits.
    Valid = Entry.Present;
    Write = Entry.Write;
    Supervisor = Entry.Supervisor;
    WriteThrough = Entry.PageLevelWriteThrough;
    CacheDisabled = Entry.PageLevelCacheDisable;
    Accessed = Entry.Accessed;
    Dirty = PageUtils::IsBitSet(Entry.Flags, DirtyBit);
    LargePage = PageUtils::IsBitSet(Entry.Flags, LargePageBit);
    Global = PageUtils::IsBitSet(Entry.Flags, GlobalBit);
    PFN = Entry.PageFrameNumber;
    ExecuteDisable = Entry.ExecuteDisable;

    // Software bits.
    Sw_Prototype = PageUtils::IsBitSet(Entry.Flags, Sw_PrototypeBit);
    Sw_Transition = PageUtils::IsBitSet(Entry.Flags, Sw_TransitionBt);

    // Strings.
    StringPFN = std::format("0x{:X}", uint64_t(PFN));
    StringPhysicalAddress = std::format("0x{:X}", PhysicalAddress);
    StringPageName = PageUtils::FormatPageTableEntry(Type, Index, Range);
}
