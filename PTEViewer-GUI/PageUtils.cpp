#include "PageUtils.h"
#include <format>
#include "../IA32.h"
#include "../Shared.h"
#include "Globals.h"
#include "PTViewerUI.h"

constexpr uint64_t VIRTUAL_ADDRESS_MASK = 0xFFFFFFFFFFFFULL;
constexpr uint64_t SIGN_EXTENSION_MASK = 0xFFFF000000000000;

#define PML4_INDEX_SHIFT 39
#define PDPT_INDEX_SHIFT 30
#define PD_INDEX_SHIFT 21
#define PT_INDEX_SHIFT 12

#define PML4_INDEX_MASK 0x1FF
#define PDPT_INDEX_MASK 0x1FF
#define PD_INDEX_MASK 0x1FF
#define PT_INDEX_MASK 0x1FF

namespace PageUtils
{
    uint64_t SignExtend48BitAddress(uint64_t Address)
    {
        if (Address & (1ULL << 47))
            Address |= SIGN_EXTENSION_MASK;

        return Address;
    }
    void PML4IndexToAddressRange(uint64_t PML4Index, AddressRange& Range)
    {
        Range.StartAddress = (PML4Index & PML4_INDEX_MASK) << PML4_INDEX_SHIFT;
        Range.EndAddress = Range.StartAddress | ((1ULL << PML4_INDEX_SHIFT) - 1);
        Range.StartAddress = SignExtend48BitAddress(Range.StartAddress);
        Range.EndAddress = SignExtend48BitAddress(Range.EndAddress);
    }
    void PDPTIndexToAddressRange(uint64_t PML4Index, uint64_t PDPTIndex, AddressRange& Range)
    {
        Range.StartAddress = ((PML4Index & PML4_INDEX_MASK) << PML4_INDEX_SHIFT) |
            ((PDPTIndex & PDPT_INDEX_MASK) << PDPT_INDEX_SHIFT);
        Range.EndAddress = Range.StartAddress | ((1ULL << PDPT_INDEX_SHIFT) - 1);
        Range.StartAddress = SignExtend48BitAddress(Range.StartAddress);
        Range.EndAddress = SignExtend48BitAddress(Range.EndAddress);
    }
    void PDIndexToAddressRange(uint64_t PML4Index, uint64_t PDPTIndex, uint64_t PDIndex, AddressRange& Range)
    {
        Range.StartAddress = ((PML4Index & PML4_INDEX_MASK) << PML4_INDEX_SHIFT) |
            ((PDPTIndex & PDPT_INDEX_MASK) << PDPT_INDEX_SHIFT) |
            ((PDIndex & PD_INDEX_MASK) << PD_INDEX_SHIFT);
        Range.EndAddress = Range.StartAddress | ((1ULL << PD_INDEX_SHIFT) - 1);
        Range.StartAddress = SignExtend48BitAddress(Range.StartAddress);
        Range.EndAddress = SignExtend48BitAddress(Range.EndAddress);
    }
    void PTIndexToAddressRange(uint64_t PML4Index, uint64_t PDPTIndex, uint64_t PDIndex, uint64_t PTIndex, AddressRange& Range)
    {
        Range.StartAddress = ((PML4Index & PML4_INDEX_MASK) << PML4_INDEX_SHIFT) |
            ((PDPTIndex & PDPT_INDEX_MASK) << PDPT_INDEX_SHIFT) |
            ((PDIndex & PD_INDEX_MASK) << PD_INDEX_SHIFT) |
            ((PTIndex & PT_INDEX_MASK) << PT_INDEX_SHIFT);
        Range.EndAddress = Range.StartAddress | ((1ULL << PT_INDEX_SHIFT) - 1);
        Range.StartAddress = SignExtend48BitAddress(Range.StartAddress);
        Range.EndAddress = SignExtend48BitAddress(Range.EndAddress);
    }

    bool IndexMatchesTarget(PTEntryType Type, const PTIndexes& Index, const PTIndexes& Target)
    {
        switch (Type)
        {
        case PTEntryType::PML4:
            return Index.PML4Index == Target.PML4Index;
        case PTEntryType::PDPT:
            return Index.PDPTIndex == Target.PDPTIndex && Index.PML4Index == Target.PML4Index;
        case PTEntryType::PD:
            return Index.PDIndex == Target.PDIndex && Index.PDPTIndex == Target.PDPTIndex && Index.PML4Index == Target.PML4Index;
        case PTEntryType::PT:
            return Index.PTIndex == Target.PTIndex && Index.PDIndex == Target.PDIndex && Index.PDPTIndex == Target.PDPTIndex && Index.PML4Index == Target.PML4Index;
        default:
            return false;
        }
    }
    AddressRange IndexToAddressRange(const PTIndexes& Index, PTEntryType Type)
    {
        AddressRange Result = { 0 };

        switch (Type)
        {
        case PTEntryType::PML4:
            PML4IndexToAddressRange(Index.PML4Index, Result);
            break;
        case PTEntryType::PDPT:
            PDPTIndexToAddressRange(Index.PML4Index, Index.PDPTIndex, Result);
            break;
        case PTEntryType::PD:
            PDIndexToAddressRange(Index.PML4Index, Index.PDPTIndex, Index.PDIndex, Result);
            break;
        case PTEntryType::PT:
            PTIndexToAddressRange(Index.PML4Index, Index.PDPTIndex, Index.PDIndex, Index.PTIndex, Result);
            break;
        }

        return Result;
    }

    std::string FormatPageTableEntry(PTEntryType Type, const PTIndexes& Index, const AddressRange& Range)
    {
        switch (Type)
        {
        case PTEntryType::PML4:
            return std::format("PML4[{}] -> 0x{:X}-0x{:X}", Index.PML4Index, Range.StartAddress, Range.EndAddress);
        case PTEntryType::PDPT:
            return std::format("PDPT[{}] -> 0x{:X}-0x{:X}", Index.PDPTIndex, Range.StartAddress, Range.EndAddress);
        case PTEntryType::PD:
            return std::format("PD[{}] -> 0x{:X}-0x{:X}", Index.PDIndex, Range.StartAddress, Range.EndAddress);
        case PTEntryType::PT:
            return std::format("PT[{}] -> 0x{:X}-0x{:X}", Index.PTIndex, Range.StartAddress, Range.EndAddress);
        default:
            return "Unknown";
        }
    }

    void TranslateAddressToPTE(uint64_t VirtualAddress, PTIndexes& IndexData)
    {
        IndexData.PML4Index = (VirtualAddress >> PML4_INDEX_SHIFT) & PML4_INDEX_MASK;
        IndexData.PDPTIndex = (VirtualAddress >> PDPT_INDEX_SHIFT) & PDPT_INDEX_MASK;
        IndexData.PDIndex = (VirtualAddress >> PD_INDEX_SHIFT) & PD_INDEX_MASK;
        IndexData.PTIndex = (VirtualAddress >> PT_INDEX_SHIFT) & PT_INDEX_MASK;
    }

    bool IsBitSet(uint64_t Number, uint8_t BitPosition)
    {
        if (BitPosition >= 64)
        {
            return false;
        }

        uint64_t Mask = 1ULL << BitPosition;
        return (Number & Mask) != 0;
    }
    const std::string BoolToString(bool Value)
    {
        return Value ? "TRUE" : "FALSE";
    }
    bool PopulateNextEntries(const PML4E_64& Entry, PTEntryType Type, QUERYPAGETABLE& NextEntries)
    {
        if (!Entry.PageFrameNumber || !Entry.Present || Type == PTEntryType::PT)
            return true;

        if (Type >= PTEntryType::PDPT && PageUtils::IsBitSet(Entry.Flags, LargePageBit))
            return true;

        if (PageUtils::IsBitSet(Entry.Flags, Sw_PrototypeBit) || PageUtils::IsBitSet(Entry.Flags, Sw_TransitionBt))
            return true;

        Globals::GetDriver()->QueryPageTable(Entry.PageFrameNumber << PAGE_SHIFT, &NextEntries);
        for (int i = 0; i < PML4E_ENTRY_COUNT_64; i++)
        {
            if (NextEntries.PML4[i].Flags)
            {
                return false;
            }
        }

        return true;
    }
}
