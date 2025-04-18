#include "mold.h"
#include "config.h"

namespace mold::pe {

std::string get_mold_version() {
  if (mold_git_hash.empty())
    return "mold/pe "s + MOLD_VERSION + " (compatible with MSVC link)";
  return "mold/pe "s + MOLD_VERSION + " (" + mold_git_hash +
         "; compatible with MSVC link)";
}

} // namespace mold::elf
