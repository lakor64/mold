#pragma once

#include "pe.h"
#include "../common/common.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/spin_mutex.h>
#include <tbb/task_group.h>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
# include <unistd.h>
#endif

namespace mold::pe {

template <typename E> class InputFile;
template <typename E> class InputSection;
template <typename E> class Symbol;
template <typename E> struct Context;
template <typename E> class ObjectFile;
template <typename E> struct Context;
template <typename E> class OutputMSDOS;
template <typename E> class OutputPEHeader;
template <typename E> class MergedSection;
template <typename E> class OutputSection;
template <typename E> class SharedFile;
template <typename E> class Symbol;
template <typename E> class RelocSection;

std::string get_mold_version();

// Chunk represents a contiguous region in an output file.
template <typename E>
class Chunk {
public:
  virtual ~Chunk() = default;
  virtual void copy_buf(Context<E> &ctx) {}
  virtual size_t size() const { return 0; }
  virtual void update_shdr(Context<E> &ctx) {}

  std::string_view name;
};

// MSDOS (MZ) stub
template <typename E>
class OutputMSDOS : public Chunk<E>
{
public:
  OutputMSDOS(Context<E> &ctx, const std::string &stub_filename) : mf(nullptr)
  {
    this->name = "MSDOS";
    load_stub(ctx, stub_filename);
  }

  OutputMSDOS();

  void copy_buf(Context<E> &ctx) override;
  size_t size() const override;

private:
  void load_stub(Context<E> &ctx, const std::string &filename);

  std::string_view contents;
  MappedFile* mf;
  MZHeader mz;
};

template <typename E>
class OutputPEHeader : public Chunk<E>
{
public:
    OutputPEHeader();

    void copy_buf(Context<E>& ctx) override;
    size_t size() const override;
    void update_shdr(Context<E> &ctx) override;

    COFFHeader header;
    COFFOptionalHeader opt;
    PEWindowsHeader<E> win;
    PEDataDirectories dirs;

private:
    ul32 magic;
}; 

// .text, .data, .bss
template <typename E>
class OutputSection : public Chunk<E>
{
  OutputSection(std::string_view name, ul32 flags) {
    this->name = name;
    hdr.Characteristics = hdr;
  }

  void copy_buf(Context<E> &ctx) override;
  size_t size() const override;
  void update_shdr(Context<E> &ctx) override;

  COFFSectionHeader hdr;
}


// InputFile is the base class of ObjectFile and SharedFile.
template <typename E>
class InputFile {
public:
  InputFile(Context<E> &ctx, MappedFile *mf);
  InputFile() : filename("<internal>") {}

  virtual ~InputFile() = default;

  COFFHeader &get_header() { return *(COFFHeader*)get_header_buf(); }
  COFFOptionalHeader &get_opt_header() { return *(COFFOptionalHeader*)get_opt_header_buf(); }
  
  MappedFile *mf = nullptr;
  std::string filename;
  bool is_dso = false;
  i64 priority;
  Atomic<bool> is_alive = false;
  std::span<COFFSectionHeader> coff_sections;
  std::span<COFFSymbolEntry> coff_symbols;
  std::vector<std::string_view> coff_strings;

  inline u8* get_header_buf() { return mf->data + coff_header_pos; }
  inline u8* get_opt_header_buf() { return get_header_buf() + sizeof(COFFHeader); }
  inline u8* get_extra_header_buf() { return get_opt_header_buf() + sizeof(COFFOptionalHeader); }
  inline u8* get_sections_buf() { return get_opt_header_buf() + get_header().SizeOfOptionalHeader; }

protected:
  void parse_stringtable();

  i64 coff_header_pos;
};

// InputSection represents a section in an input object file.
template <typename E>
class InputSection {
public:
  InputSection(Context<E> &ctx, ObjectFile<E> &file, i64 shndx);

  std::string_view name;
  const COFFSectionHeader &shdr() const;
  
  ObjectFile<E> &file;

  std::string_view contents;
  std::span<COFFRelocation> relocs;
  std::span<COFFLineEntry> lines;

  i32 shndx = -1;
};

// ObjectFile represents an input .obj file.
template <typename E>
class ObjectFile : public InputFile<E> {
public:
  ObjectFile() = default;

  static ObjectFile<E> *create(Context<E> &ctx, MappedFile *mf,
                               std::string archive_name, bool is_in_lib);

  void parse(Context<E> &ctx);

  std::string archive_name;
  std::vector<std::unique_ptr<InputSection<E>>> sections;
  bool is_in_lib = false;

private:
  void initialize_sections(Context<E> &ctx);

  ObjectFile(Context<E> &ctx, MappedFile *mf,
             std::string archive_name, bool is_in_lib);

};


template <typename E>
class MergedSection : public Chunk<E> {
public:

};

// Context represents a context object for each invocation of the linker.
// It contains command line flags, pointers to singleton objects
// (such as linker-synthesized output sections), unique_ptrs for
// resource management, and other miscellaneous objects.
template <typename E>
struct Context {

  Context() {
  }

  Context(const Context<E> &) = delete;

  void checkpoint() {
    if (has_error) {
      cleanup();
      _exit(1);
    }
  }

  // Command-line arguments
  struct {
    bool color_diagnostics = false;
    bool demangle = true;
    bool fork = true;
    bool quick_exit = true;
    bool trace = false;
    bool suppress_warnings = false;
    bool fatal_warnings = false;
    bool dll = false;
    bool large_address = true;
    i64 filler = -1;
    i64 thread_count = 0;
    i64 align = -1;
    i64 base = -1;
    i64 filealign = -1;
    i64 heap = -1;
    i64 stack;
    i64 subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    i64 subsystem_major_version = 0;
    i64 subsystem_minor_version = 0;
    i64 image_major_version = 0;
    i64 image_minor_version = 0;
    std::string_view emulation;
    std::string chroot;
    std::string plugin;
    std::string output = "a.exe"; // TODO: this should be automatically adjusted to the last source file / according to what link.exe does
    std::string stub = "";
    std::vector<std::string> library_paths;
  } arg;

  // Reader context
  std::unordered_set<std::string_view> visited;
  bool in_lib = false;
  i64 file_priority = 10000;
  tbb::task_group tg;

  bool has_error = false;

  tbb::concurrent_vector<std::unique_ptr<TimerRecord>> timer_records;
  tbb::concurrent_vector<std::function<void()>> on_exit;

  // File pools
  tbb::concurrent_vector<std::unique_ptr<ObjectFile<E>>> obj_pool;
  tbb::concurrent_vector<std::unique_ptr<MappedFile>> mf_pool;
  tbb::concurrent_vector<std::unique_ptr<Chunk<E>>> chunk_pool;

  // Fully-expanded command line args
  std::vector<std::string_view> cmdline_args;

  // Input files
  std::vector<ObjectFile<E> *> objs;

  // Output buffer
  std::unique_ptr<OutputFile<Context<E>>> output_file;
  u8 *buf = nullptr;

  std::vector<Chunk<E> *> chunks;

  // Output chunks
  OutputMSDOS<E> *msdos = nullptr;
  OutputPEHeader<E> *pe = nullptr;
};

//
// cmdline.cc
//

template <typename E>
std::vector<std::string_view>
pack_args(Context<E> &ctx, char **argv);

template <typename E>
std::vector<std::string> parse_nonpositional_args(Context<E> &ctx);

template <typename E>
std::ostream& operator<<(std::ostream& out, const InputFile<E>& file);

template <typename E>
int pe_main(int argc, char **argv);

int main(int argc, char **argv);

//
// passes.cc
//
template <typename E> int redo_main(Context<E> &, int argc, char **argv);
template <typename E> void create_synthetic_sections(Context<E> &);
template <typename E> void copy_chunks(Context<E> &);
template <typename E> i64 set_osec_offsets(Context<E> &);
template <typename E> void compute_section_headers(Context<E> &);
template <typename E> void compute_grouped_sections(Context<E> &);



//
// Inline objects and functions
//

template <typename E>
inline std::ostream&
operator<<(std::ostream& out, const InputSection<E>& isec) {
    out << isec.file << ":(" << isec.name() << ")";
    return out;
}

template <typename E>
inline const COFFSectionHeader& InputSection<E>::shdr() const {
    return file.coff_sections[shndx];
}
}