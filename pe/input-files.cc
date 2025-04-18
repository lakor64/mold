#include "mold.h"

namespace mold::pe {


template <typename E>
InputFile<E>::InputFile(Context<E> &ctx, MappedFile *mf)
  : mf(mf), filename(mf->name) {
  if (mf->size < sizeof(COFFHeader))
    Fatal(ctx) << *this << ": file too small";

  coff_header_pos = 0; // TODO: add check for PE/MZ magic and adjust this accordingly

  COFFHeader &ehdr = *(COFFHeader*)get_header_buf();

  COFFSectionHeader *sh_begin = (COFFSectionHeader*)get_sections_buf();

  ul16 num_sections = ehdr.NumberOfSections;

  if (mf->data + mf->size < (u8 *)(sh_begin + num_sections))
    Fatal(ctx) << mf->name << ": NumberOfSections of COFF Image Header seems corrupted: "
               << mf->size << " " << num_sections;
  coff_sections = {sh_begin, sh_begin + num_sections};

  // parse symbol table

  if (ehdr.PointerToSymbolTable != 0 && ehdr.NumberOfSymbols > 0)
  {
    COFFSymbolEntry* sym_beg = (COFFSymbolEntry*)(mf->data + ehdr.PointerToSymbolTable);
    coff_symbols = { sym_beg, sym_beg + ehdr.NumberOfSymbols};
  }

  if (mf->size > (ehdr.PointerToSymbolTable + ehdr.NumberOfSymbols * sizeof(COFFSymbolEntry)))
      parse_stringtable();
}

template <typename E>
void InputFile<E>::parse_stringtable()
{
    const COFFHeader& ehdr = get_header();
    u8* st = mf->data + ehdr.PointerToSymbolTable + (ehdr.NumberOfSymbols * sizeof(COFFSymbolEntry));

  ul32 length = *(ul32*)st;
  length -= 4;
  st += sizeof(ul32);
  for (ul32 i = 0; i < length; )
  {
    std::string_view q = (const char*)(st + i);
    i += q.size() + 1;
    coff_strings.emplace_back(q);
  }
}

template <typename E>
ObjectFile<E>::ObjectFile(Context<E> &ctx, MappedFile *mf,
                          std::string archive_name, bool is_in_lib)
  : InputFile<E>(ctx, mf), archive_name(archive_name), is_in_lib(is_in_lib) {
  this->is_alive = !is_in_lib;
}

template <typename E>
ObjectFile<E> *
ObjectFile<E>::create(Context<E> &ctx, MappedFile *mf,
                      std::string archive_name, bool is_in_lib) {
  ObjectFile<E> *obj = new ObjectFile<E>(ctx, mf, archive_name, is_in_lib);
  ctx.obj_pool.emplace_back(obj);
  return obj;
}

template <typename E>
void ObjectFile<E>::parse(Context<E> &ctx) {
  sections.resize(this->coff_sections.size());

  initialize_sections(ctx);
}

template <typename E>
void ObjectFile<E>::initialize_sections(Context<E> &ctx) {
  for (i64 i = 0; i < this->coff_sections.size(); i++) {
    const COFFSectionHeader &shdr = this->coff_sections[i];
    std::string_view name = std::string_view((const char*)shdr.Name, 8);
    if (shdr.Name[0] == '/') // PE long name
    {
    char name_str[8] = {0};
    uint64_t pos;

    memcpy(name_str, shdr.Name + 1, 7);
    pos = (uint64_t)std::atoll(name_str);

    name = (const char*)(mf->data + get_header().PointerToSymbolTable + (get_header().NumberOfSymbols * sizeof(COFFSymbolEntry)) + pos);
    }

    this->sections[i] = std::make_unique<InputSection<E>>(ctx, *this, i);
    this->sections[i]->name = name;

    // attach relocations
    if (shdr.PointerToRelocations > 0 && shdr.NumberOfRelocations > 0)
    {
        COFFRelocation* reloc_data = (COFFRelocation*)(this->mf->data + shdr.PointerToRelocations);
        this->sections[i]->relocs = { reloc_data, reloc_data + shdr.NumberOfRelocations };
    }

    if (shdr.PointerToLinenumbers > 0 && shdr.NumberOfLinenumbers > 0) // usually not present in PE
    {
        COFFLineEntry* line_data = (COFFLineEntry*)(this->mf->data + shdr.PointerToLinenumbers);
        this->sections[i]->lines = { line_data, line_data + shdr.NumberOfLinenumbers };
    }

    static Counter counter("regular_sections");
    counter++;
  }
}

template <typename E>
std::ostream &operator<<(std::ostream &out, const InputFile<E> &file) {
  if (file.is_dso) {
    out << path_clean(file.filename);
    return out;
  }

  ObjectFile<E> *obj = (ObjectFile<E> *)&file;
  if (obj->archive_name == "")
    out << path_clean(obj->filename);
  else
    out << path_clean(obj->archive_name) << "(" << obj->filename + ")";
  return out;
}


using E = MOLD_TARGET;

template class InputFile<E>;
template class ObjectFile<E>;
template std::ostream &operator<<(std::ostream &, const InputFile<E> &);


} // namespace mold::pe
