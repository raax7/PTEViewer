#pragma once
#include <Windows.h>
#include "../IA32.h"
#include "../Shared.h"
#include "PollManager.h"
#include "SharedTypes.h"
#include "PageUtils.h"
#include <Windows.h>
#include <memory>
#include <tuple>
#include <unordered_map>

class PageTableCache
{
public:
    PageTableCache(uint64_t ProcessCR3);
    ~PageTableCache();

public:
    struct PTData
    {
        QUERYPAGETABLE NextEntries;

        bool IsLeaf;

        std::string StringPFN;
        std::string StringPhysicalAddress;
        std::string StringPageName;

        // Extra/Custom.
        uint64_t PhysicalAddress;

        // Common page bits.
        uint8_t Valid : 1;
        uint8_t Write : 1;
        uint8_t Supervisor : 1;
        uint8_t WriteThrough : 1;
        uint8_t CacheDisabled : 1;
        uint8_t Accessed : 1;
        uint8_t Dirty : 1;
        uint8_t LargePage : 1;
        uint8_t Global : 1;
        uint64_t PFN : 36;
        uint8_t ExecuteDisable : 1;

        // Software-specific bits.
        uint8_t Sw_Prototype : 1;
        uint8_t Sw_Transition : 1;

        PTData() = default;
        PTData(const PML4E_64& Entry, const PageUtils::PTIndexes& Index, const PageUtils::AddressRange& Range, PTEntryType Type, QUERYPAGETABLE& NextEntries);
    };

public:
    using EntryKey = std::tuple<int, int, int, int, PTEntryType>;

    void CacheEntry(const EntryKey& Key, const PTData& PTData);
    bool TryGetEntry(const EntryKey& Key, PTData& PTData);
    void Clear();

    PML4E_64* GetPML4() { return m_PML4; }

private:
    void PollCallback();

private:
    struct EntryKeyHash
    {
        std::size_t operator()(const EntryKey& key) const
        {
            std::size_t h1 = std::hash<int>{}(std::get<0>(key));
            std::size_t h2 = std::hash<int>{}(std::get<1>(key));
            std::size_t h3 = std::hash<int>{}(std::get<2>(key));
            std::size_t h4 = std::hash<int>{}(std::get<3>(key));
            std::size_t h5 = std::hash<int>{}(static_cast<int>(std::get<4>(key)));

            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
        }
    };

    struct EntryKeyEqual
    {
        bool operator()(const EntryKey& lhs, const EntryKey& rhs) const
        {
            return lhs == rhs;
        }
    };

    std::unordered_map<EntryKey, PTData, EntryKeyHash, EntryKeyEqual> m_Cache;
    std::shared_ptr<PollEntry> m_PollEntry;

    uint64_t m_ProcessCR3;
    PML4E_64 m_PML4[PML4E_ENTRY_COUNT_64];
};
