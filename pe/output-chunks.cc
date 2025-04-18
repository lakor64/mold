#include "mold.h"
#include "msdos_stub.h"
#include "../common/filetype.h"

namespace mold::pe {
    template <typename E>
    OutputMSDOS<E>::OutputMSDOS() : mf(nullptr)
    {
        this->name = "MSDOS";
        this->mz = *(MZHeader*)dosstub;
        i64 header_size = (mz.e_cparhdr * 16);
        this->contents = std::string_view(dosstub + header_size, dosstub_len - header_size);

        memset(&mz.e_res1, 0, sizeof(mz.e_res1));
        memset(&mz.e_res2, 0, sizeof(mz.e_res2));
        mz.e_oemid = 0;
        mz.e_oeminfo = 0;
        mz.e_lfanew = 120;
    }

    template <typename E>
    void OutputMSDOS<E>::load_stub(Context<E> &ctx, const std::string &path)
    {
        mf = must_open_file(ctx, path);

        if (get_file_type(ctx, mf) != FileType::MZ_EXE)
        {
            Fatal(ctx) << "MS-DOS Stub " << path << " must be a valid MZ file";
        }

        mz = *(MZHeader*)mf->data;

        if ((mz.e_cparhdr * 16) > mf->size)
        {
            Fatal(ctx) << "MS-DOS Stub " << path << " contains an invalid header";
        }

        // We don't properly know if the file has the PE extensions or not
        memset(&mz.e_res1, 0, sizeof(mz.e_res1));
        memset(&mz.e_res2, 0, sizeof(mz.e_res2));
        mz.e_oemid = 0;
        mz.e_oeminfo = 0;
        mz.e_lfanew = mf->size; // set PE offset

        i64 header_size = (mz.e_cparhdr * 16);

        contents = std::string_view(mf->get_contents().data() + header_size, mf->size - header_size);
    }

    template <typename E>
    size_t OutputMSDOS<E>::size() const {
        i64 header_size = (mz.e_cparhdr * 16);
        return header_size + contents.size();
    }

    template <typename E>
    void OutputMSDOS<E>::copy_buf(Context<E> &ctx) {
        // simply copy the ms-dos stub
        i64 header_size = (mz.e_cparhdr * 16);
        memcpy(ctx.buf, &mz, header_size);
        memcpy(ctx.buf + header_size, contents.data(), contents.size());
    }

    template <typename E>
    OutputPEHeader<E>::OutputPEHeader() : magic(0) {
        this->name = "PE";
        magic = 0x4550; // PE\0\0

        // Optional header magic
        if constexpr (E::is_64)
        {
            opt.Magic = 0x20b;
        }
        else
        {
            opt.Magic = 0x10b;
        }

        // TODO: Setup major/minor linker version
        opt.MajorLinkerVersion = 0;
        opt.MinorLinkerVersion = 0;
    }

    template <typename E>
    size_t OutputPEHeader<E>::size() const {
        return sizeof(magic) + sizeof(COFFHeader) + sizeof(COFFOptionalHeader) + sizeof(PEWindowsHeader<E>);
    }

    template <typename E>
    void OutputPEHeader<E>::update_shdr(Context<E> &ctx)
    {
        header.Magic = E::MachineType;
        header.NumberOfSections = 0;
        header.TimeDateStamp = 0; // Get date
        header.PointerToSymbolTable = 0;
        header.NumberOfSymbols = 0;
        header.SizeOfOptionalHeader = sizeof(opt) + sizeof(win) + sizeof(dirs);
        header.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE; // DLL/EXE have this setted up

        if constexpr (!E::is_64)
            header.Characteristics |= IMAGE_FILE_32BIT_MACHINE;
        
        if (ctx.arg.large_address)
            header.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
        
        if (ctx.arg.dll)
            header.Characteristics |= IMAGE_FILE_DLL;

        opt.SizeOfCode = 0; // sizeof .text
        opt.SizeOfInitializedData = 0; // sizeof .data
        opt.SizeOfUninitializedData = 0; // sizeof .bss
        opt.AddressOfEntryPoint = 0; // eip position
        opt.BaseOfCode = 0; // memory base .text (0x4000)?

        if constexpr (!E::is_64)
        {
            opt.BaseOfData = 0; // memory base .data (??)
        }

        win.ImageBase = ctx.arg.base;

        // TODO: implement
        win.SectionAlignment = 4096;
        win.FileAlignment = 512;
        // --

        win.MajorOperatingSystemVersion = 0;
        win.MinorOperatingSystemVersion = 0;
        win.MajorImageVersion = ctx.arg.image_major_version;
        win.MinorImageVersion = ctx.arg.image_minor_version;
        win.MajorSubsystemVersion = ctx.arg.subsystem_major_version;
        win.MinorSubsystemVersion = ctx.arg.subsystem_minor_version;
        win.Win32VersionValue = 0;
        win.SizeOfHeaders = header.SizeOfOptionalHeader + ctx.msdos->size();
        win.SizeOfImage = win.SizeOfHeaders;
        win.CheckSum = 0;
        win.Subsystem = ctx.arg.subsystem;
        win.DllCharacteristics = 0;
        win.SizeOfStackReserve = 0;
        win.SizeOfStackCommit = 0;
        win.SizeOfHeapReserve = 0;
        win.SizeOfHeapCommit = 0;
        win.LoaderFlags = 0;
        win.NumberOfRvaAndSizes = sizeof(PEDataDirectories) / sizeof(PEDataDirectory);

        memset(&dirs, 0, sizeof(dirs));
    }

    template <typename E>
    void OutputPEHeader<E>::copy_buf(Context<E>& ctx)
    {
        i64 offset = ctx.msdos->size();
        memcpy(ctx.buf + offset, &magic, sizeof(magic)); // PE magic
        offset += sizeof(magic);
        memcpy(ctx.buf + offset, &header, sizeof(header));
        offset += sizeof(header);
        memcpy(ctx.buf + offset, &opt, sizeof(opt));
        offset += sizeof(opt);
        memcpy(ctx.buf + offset, &win, sizeof(win));
        offset += sizeof(win);
        memcpy(ctx.buf + offset, &dirs, sizeof(dirs));
    }

    using E = MOLD_TARGET;
    template class Chunk<E>;
    template class OutputMSDOS<E>;
    template class OutputPEHeader<E>;
}
