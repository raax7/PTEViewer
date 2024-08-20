#pragma once
#include <cstdint>
#include <string>
#include <Windows.h>
#include "SharedTypes.h"
#include "../IA32.h"
#include "../Shared.h"

namespace PageUtils
{
    struct AddressRange
    {
        uint64_t StartAddress;
        uint64_t EndAddress;
    };

    struct PTIndexes
    {
        uint16_t PML4Index;
        uint16_t PDPTIndex;
        uint16_t PDIndex;
        uint16_t PTIndex;
    };
}

namespace PageUtils
{
    // Address manipulation.
    //
    uint64_t SignExtend48BitAddress(uint64_t Address);
    void PML4IndexToAddressRange(uint64_t PML4Index, AddressRange& Range);
    void PDPTIndexToAddressRange(uint64_t PML4Index, uint64_t PDPTIndex, AddressRange& Range);
    void PDIndexToAddressRange(uint64_t PML4Index, uint64_t PDPTIndex, uint64_t PDIndex, AddressRange& Range);
    void PTIndexToAddressRange(uint64_t PML4Index, uint64_t PDPTIndex, uint64_t PDIndex, uint64_t PTIndex, AddressRange& Range);

    // Index manipulation.
    //
    bool IndexMatchesTarget(PTEntryType Type, const PageUtils::PTIndexes& Index, const PageUtils::PTIndexes& Target);
    PageUtils::AddressRange IndexToAddressRange(const PageUtils::PTIndexes& Index, PTEntryType Type);

    // Formatting.
    //
    std::string FormatPageTableEntry(PTEntryType Type, const PageUtils::PTIndexes& Index, const PageUtils::AddressRange& Range);

    // Translation.
    //
    void TranslateAddressToPTE(uint64_t VirtualAddress, PageUtils::PTIndexes& IndexData);

    // Misc.
    //
    bool IsBitSet(uint64_t Number, uint8_t BitPosition);
    const std::string BoolToString(bool Value);
    bool PopulateNextEntries(const PML4E_64& Entry, PTEntryType Type, QUERYPAGETABLE& NextEntries);
}
