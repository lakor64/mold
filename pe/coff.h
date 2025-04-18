/*
    This file contains PE/COFF object formats.
*/
#pragma once

#include "../../common/integers.h"

#include <string_view>
#include <type_traits>
#include <concepts>

namespace mold::pe {

    // COFF object header
    struct COFFHeader
    {
        ul16 Magic; // Under PE, this is the Machine type as PE doesn't have a COFF magic
        ul16 NumberOfSections;
        ul32 TimeDateStamp;
        ul32 PointerToSymbolTable;
        ul32 NumberOfSymbols;
        ul16 SizeOfOptionalHeader;
        ul16 Characteristics;
    };

    struct COFFOptionalHeader
    {
        ul16 Magic;
        u8 MajorLinkerVersion;
        u8 MinorLinkerVersion;
        ul32 SizeOfCode;
        ul32 SizeOfInitializedData;
        ul32 SizeOfUninitializedData;
        ul32 AddressOfEntryPoint;
        ul32 BaseOfCode;
        ul32 BaseOfData;
    };

    struct COFFSectionHeader // section header
    {
        u8 Name[8];
        ul32 VirtualSize;
        ul32 VirtualAddress;
        ul32 SizeOfRawData;
        ul32 PointerToRawData;
        ul32 PointerToRelocations;
        ul32 PointerToLinenumbers;
        ul16 NumberOfRelocations;
        ul16 NumberOfLinenumbers;
        ul32 Characteristics;
    };

    struct COFFSymbolName
    {
        union
        {
            char ShortName[8];
            struct
            {
                ul32 Zeroes;
                ul32 Offset;
            };
        };
    };

    struct COFFSymbolEntry
    {
        COFFSymbolName Name;
        ul32 Value;
        ul16 SectionNumber;
        ul16 Type;
        u8 StorageClass;
        u8 NumberOfAuxSymbols;
    };

    struct COFFLineEntry
    {
        union
        {
            ul32 SymbolTableIndex;
            ul32 VirtualAddress;
        } Addr;
        ul16 LineNumber;
    };

    struct COFFRelocation
    {
        ul32 VirtualAddress;
        ul32 SymbolTableIndex;
        ul16 Type;
    };
}
