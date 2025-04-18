#include "mold.h"

namespace mold::pe {

template <typename E>
InputSection<E>::InputSection(Context<E> &ctx, ObjectFile<E> &file, i64 shndx)
  : file(file), shndx(shndx) {
  if (shndx < file.sections.size())
    contents = {(char*)file.mf->data + shdr().PointerToRawData, (size_t)shdr().SizeOfRawData };
}


using E = MOLD_TARGET;

template class InputSection<E>;

} // namespace mold::pe
