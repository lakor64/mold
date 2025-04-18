# mold backend for PE/COFF

The format is used in various Microsoft products, including Windows, Windows 9x, Xbox and .NET.

The linker was designed to be a drop-in replacement for Microsoft Linker (LINK.EXE).

## Goals
- Windows XP or greater
- x86_64 working linker

## Future goals
- Extend to get i386 working as well (arm64 also?)
- Support for /KERNEL, /DRIVER (which binutils lacks)
- Support for PDB generation
- Support for delay loads (which binutils lack)
- Support the rest of sections
- Support the rest of cli from the linker

## Irrealistic goals
- CLR (This would mean support MSIL images and exporting PE with MSIL images, which sound very irrealistic)
- Windows CE (/WINDOWSCE)
- PowerPC (RS/6000 LE), MIPS R4000/R10000, SH4, SH5, ARM32, PowerPC BE (Very irrealistic goal but it would mean an open-source linker for Xbox360)
- Windows 9x (VXD+EXE) (/VXD, potentially means getting MSVC4/OWCC libraries as input?)
- Windows NT4/NT5.0
- Support for mingw32 gcc specific outputs (clang outputs should be ok as I remember you can use clang+ms link.exe)

## Command line

MSVC linker CLI differences between standard GCC CLI:
- All commands are case-insensitivie
- Params are passed with /XXXX and -XXXX
- No --XXXX param exist
- Arguments after the params are readed by ":" and not by "="

Mold will NOT support the behavour /IGNORE: flag due to Mold not having anything for Error code references that we can emulate. The linker will accept and ignore and argument that is not "all", altrough it will not emit a warning for comaptibility reasons.

### Supported command lines
- /?
- /base
- /dll (wip, no binary generation supported yet)
- /largeaddressaware
- /libpath (wip, no .lib pasing supported)
- /machine
- /nologo
- /out
- /stub
- /subsystem
- /version
- /wx

### Custom Mold command line params
- /fork
- /ignore:all
- /quickexit
- /threadcount
- /verbose

## PE Differences

### MS-DOS STUB
The MS-DOS is a simple DOS application that prints a message and exists. The stub can be modified via /STUB

## References
- [Microsoft official specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
- [Microsoft Image file constants](https://learn.microsoft.com/en-us/windows/win32/sysinfo/image-file-machine-constants)
- [Microsoft Linker options](https://learn.microsoft.com/en-us/cpp/build/reference/linker-options?view=msvc-170)
