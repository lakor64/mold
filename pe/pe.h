#pragma once

#include "coff.h"

#include <string_view>

namespace mold::pe {

struct X86_64;

// native LE PE size
template <typename T>
using uln = std::conditional_t<T::is_64, ul64, ul32>;

enum : u32 {
#ifndef _WINNT_
    IMAGE_FILE_MACHINE_UNKNOWN      = 0x0000,
    IMAGE_FILE_MACHINE_ALPHA        = 0x0184, // AXP32
    IMAGE_FILE_MACHINE_ALPHA64      = 0x0284, // AXP64
    IMAGE_FILE_MACHINE_AXP64        = IMAGE_FILE_MACHINE_ALPHA64,
    IMAGE_FILE_MACHINE_AM33         = 0x01d3,
    IMAGE_FILE_MACHINE_I386         = 0x014c, // i386
    IMAGE_FILE_MACHINE_AMD64        = 0x8664, // x86_64
    IMAGE_FILE_MACHINE_ARM          = 0x01c0, // ARM32 LE
    IMAGE_FILE_MACHINE_THUMB        = 0x01c2, // ARM THUMB-1 LE
    IMAGE_FILE_MACHINE_ARMNT        = 0x01c4, // ARM32 THUMB-2
    IMAGE_FILE_MACHINE_ARM64        = 0xaa64, // AARCH64 LE
    IMAGE_FILE_MACHINE_CEE          = 0xc0ee,
    IMAGE_FILE_MACHINE_CEF          = 0x0cef,
    IMAGE_FILE_MACHINE_EBC          = 0x0ebc, // EFI
    IMAGE_FILE_MACHINE_IA64         = 0x0200, // IA64
#endif
    IMAGE_FILE_MACHINE_LOONGARCH32  = 0x6232,
    IMAGE_FILE_MACHINE_LOONGARCH64  = 0x6264,
#ifndef _WINNT_
    IMAGE_FILE_MACHINE_M32R         = 0x9041, // M32R LE
    IMAGE_FILE_MACHINE_R3000        = 0x0162, // MIPSv2 32 LE
    IMAGE_FILE_MACHINE_R4000        = 0x0166, // MIPSv3 32 LE
    IMAGE_FILE_MACHINE_R10000       = 0x0168,
    IMAGE_FILE_MACHINE_WCEMIPSV2    = 0x0169, // MIPS32 LE
    IMAGE_FILE_MACHINE_MIPS16       = 0x0266,
    IMAGE_FILE_MACHINE_MIPSFPU      = 0x0366,
    IMAGE_FILE_MACHINE_MIPSFPU16    = 0x0466,
    IMAGE_FILE_MACHINE_POWERPC      = 0x01f0, // PPC32 LE
    IMAGE_FILE_MACHINE_POWERPCFP    = 0x01f1, // PPC32 LE + FPU
#endif
    IMAGE_FILE_MACHINE_POWERPCBE    = 0x01f2, // PPC32 BE (Xbox360)
    IMAGE_FILE_MACHINE_RISCV32      = 0x5032,
    IMAGE_FILE_MACHINE_RISCV64      = 0x5064,
    IMAGE_FILE_MACHINE_RISCV128     = 0x5128,
#ifndef _WINNT_
    IMAGE_FILE_MACHINE_SH3          = 0x01a2, // SH3 LE
    IMAGE_FILE_MACHINE_SH3DSP       = 0x01a3,
    IMAGE_FILE_MACHINE_SH3E         = 0x01a4,
    IMAGE_FILE_MACHINE_SH4          = 0x01a6, // SH4 LE
    IMAGE_FILE_MACHINE_SH5          = 0x01a8, // SH5 LE
    IMAGE_FILE_MACHINE_TRICORE      = 0x0520,
#endif
};

#ifndef _WINNT_
enum : u16 {
    IMAGE_SUBSYSTEM_UNKNOWN                     = 0,
    IMAGE_SUBSYSTEM_NATIVE                      = 1,
    IMAGE_SUBSYSTEM_WINDOWS_GUI                 = 2,
    IMAGE_SUBSYSTEM_WINDOWS_CUI                 = 3,
    IMAGE_SUBSYSTEM_OS2_CUI                     = 5,
    IMAGE_SUBSYSTEM_POSIX_CUI                   = 7,
    IMAGE_SUBSYSTEM_NATIVE_WINDOWS              = 8,
    IMAGE_SUBSYSTEM_WINDOWS_CE_GUI              = 9,
    IMAGE_SUBSYSTEM_EFI_APPLICATION             = 10,
    IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER     = 11,
    IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER          = 12,
    IMAGE_SUBSYSTEM_EFI_ROM                     = 13,
    IMAGE_SUBSYSTEM_XBOX                        = 14,
    IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION    = 16,
    IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG           = 17,
};

enum : u16 {
    IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA        = 0x0020,
    IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE           = 0x0040,
    IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY        = 0x0080,
    IMAGE_DLLCHARACTERISTICS_NX_COMPAT              = 0x0100,
    IMAGE_DLLCHARACTERISTICS_NO_ISOLATION           = 0x0200,
    IMAGE_DLLCHARACTERISTICS_NO_SEH                 = 0x0400,
    IMAGE_DLLCHARACTERISTICS_NO_BIND                = 0x0800,
    IMAGE_DLLCHARACTERISTICS_APPCONTAINER           = 0x1000,
    IMAGE_DLLCHARACTERISTICS_WDM_DRIVER             = 0x2000,
    IMAGE_DLLCHARACTERISTICS_GUARD_CF               = 0x4000,
    IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE  = 0x8000,
};
#endif

#ifndef _WINNT_
enum : u16 {
    IMAGE_FILE_RELOCS_STRIPPED              = 0x0001,
    IMAGE_FILE_EXECUTABLE_IMAGE             = 0x0002,
    IMAGE_FILE_LINE_NUMS_STRIPPED           = 0x0004,
    IMAGE_FILE_LOCAL_SYMS_STRIPPED          = 0x0008,
    IMAGE_FILE_AGGRESSIVE_WS_TRIM           = 0x0010,
    IMAGE_FILE_LARGE_ADDRESS_AWARE          = 0x0020,
    IMAGE_FILE_BYTES_REVERSED_LO            = 0x0080,
    IMAGE_FILE_32BIT_MACHINE                = 0x0100,
    IMAGE_FILE_DEBUG_STRIPPED               = 0x0200,
    IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP      = 0x0040,
    IMAGE_FILE_NET_RUN_FROM_SWAP            = 0x0800,
    IMAGE_FILE_SYSTEM                       = 0x1000,
    IMAGE_FILE_DLL                          = 0x2000,
    IMAGE_FILE_UP_SYSTEM_ONLY               = 0x4000,
    IMAGE_FILE_BYTES_REVERSED_HI            = 0x8000,
};
#endif

#ifndef _WINNT_
enum : u32 {
    IMAGE_SCN_TYPE_NO_PAD                   = 0x00000008,
    IMAGE_SCN_CNT_CODE                      = 0x00000020,
    IMAGE_SCN_CNT_INITIALIZED_DATA          = 0x00000040,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA        = 0x00000080,
    IMAGE_SCN_LNK_OTHER                     = 0x00000100,
    IMAGE_SCN_LNK_INFO                      = 0x00000200,
    IMAGE_SCN_LNK_REMOVE                    = 0x00000800,
    IMAGE_SCN_LNK_COMDAT                    = 0x00001000,
    IMAGE_SCN_GPREL                         = 0x00008000,
    IMAGE_SCN_MEM_PURGEABLE                 = 0x00020000,
    IMAGE_SCN_MEM_16BIT                     = 0x00020000,
    IMAGE_SCN_MEM_LOCKED                    = 0x00040000,
    IMAGE_SCN_MEM_PRELOAD                   = 0x00080000,
    IMAGE_SCN_ALIGN_1BYTES                  = 0x00100000,
    IMAGE_SCN_ALIGN_2BYTES                  = 0x00200000,
    IMAGE_SCN_ALIGN_4BYTES                  = 0x00300000,
    IMAGE_SCN_ALIGN_8BYTES                  = 0x00400000,
    IMAGE_SCN_ALIGN_16BYTES                 = 0x00500000,
    IMAGE_SCN_ALIGN_32BYTES                 = 0x00600000,
    IMAGE_SCN_ALIGN_64BYTES                 = 0x00700000,
    IMAGE_SCN_ALIGN_128BYTES                = 0x00800000,
    IMAGE_SCN_ALIGN_256BYTES                = 0x00900000,
    IMAGE_SCN_ALIGN_512BYTES                = 0x00A00000,
    IMAGE_SCN_ALIGN_1024BYTES               = 0x00B00000,
    IMAGE_SCN_ALIGN_2048BYTES               = 0x00C00000,
    IMAGE_SCN_ALIGN_4096BYTES               = 0x00D00000,
    IMAGE_SCN_ALIGN_8192BYTES               = 0x00E00000,
    IMAGE_SCN_LNK_NRELOC_OVFL               = 0x01000000,
    IMAGE_SCN_MEM_DISCARDABLE               = 0x02000000,
    IMAGE_SCN_MEM_NOT_CACHED                = 0x04000000,
    IMAGE_SCN_MEM_NOT_PAGED                 = 0x08000000,
    IMAGE_SCN_MEM_SHARED                    = 0x10000000,
    IMAGE_SCN_MEM_EXECUTE                   = 0x20000000,
    IMAGE_SCN_MEM_READ                      = 0x40000000,
    IMAGE_SCN_MEM_WRITE                     = 0x80000000,
};
#endif

struct X86_64 {
    static constexpr bool is_le = true;
    static constexpr bool is_64 = true;

    static constexpr std::string_view target_name = "AMD64";
    static constexpr u16 MachineType = IMAGE_FILE_MACHINE_AMD64;
};

/*
    Windows NT is a Little Endian kernel.
    No Windows PC platform runs a Windows NT Big Endian kernel.

    The Xbox360 is a modified OS from Windows NT, which uses it's own
     binary format called XEX2. Inside the XEX2 (once decrypted), one
     can find a PE much like Windows one.

    From the Xbox360 (which is the only Big Endian platform to run Windows NT)
     we learn that the PE format maintains it's original little endian
     structure expect for the relocation data (and text data obviously).
    
    Therefore, it's safe to assume that the data here will ALWAYS be Little Endian.
*/

// MSDOS header
struct MZHeader {
    ul16 e_magic;
    ul16 e_cblp;
    ul16 e_cp;
    ul16 e_crlc;
    ul16 e_cparhdr;
    ul16 e_minalloc;
    ul16 e_maxalloc;
    ul16 e_ss;
    ul16 e_sp;
    ul16 e_csum;
    ul16 e_ip;
    ul16 e_cs;
    ul16 e_lfarlc;
    ul16 e_ovno;
    ul16 e_res1[4];
    ul16 e_oemid;
    ul16 e_oeminfo;
    ul16 e_res2[10];
    il32 e_lfanew;
};

// PE/COFF specific Optional header
template <typename E>
struct PEWindowsHeader
{
    uln<E> ImageBase;
    ul32 SectionAlignment;
    ul32 FileAlignment;
    ul16 MajorOperatingSystemVersion;
    ul16 MinorOperatingSystemVersion;
    ul16 MajorImageVersion;
    ul16 MinorImageVersion;
    ul16 MajorSubsystemVersion;
    ul16 MinorSubsystemVersion;
    ul32 Win32VersionValue;
    ul32 SizeOfImage;
    ul32 SizeOfHeaders;
    ul32 CheckSum;
    ul16 Subsystem;
    ul16 DllCharacteristics;
    uln<E> SizeOfStackReserve;
    uln<E> SizeOfStackCommit;
    uln<E> SizeOfHeapReserve;
    uln<E> SizeOfHeapCommit;
    ul32 LoaderFlags;
    ul32 NumberOfRvaAndSizes;
};

struct PEDataDirectory
{
    ul32 VirtualAddress;
    ul32 Size;
};

struct PEDataDirectories
{
    PEDataDirectory ExportTable;
    PEDataDirectory ImportTable;
    PEDataDirectory ResourceTable;
    PEDataDirectory ExceptionTable;
    PEDataDirectory CertificateTable;
    PEDataDirectory BaseRelocationTable;
    PEDataDirectory Debug;
    PEDataDirectory Architecture;
    PEDataDirectory GlobalPtr;
    PEDataDirectory TLSTable;
    PEDataDirectory LoadConfigTable;
    PEDataDirectory BoundImport;
    PEDataDirectory IAT;
    PEDataDirectory DelayImportDescriptor;
    PEDataDirectory CLRRuntimeHeader;
    PEDataDirectory Reserved;
};

} // namespace mold::pe
