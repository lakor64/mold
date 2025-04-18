#include "mold.h"

#include <tbb/parallel_for_each.h>

namespace mold::pe {

// Since pe_main is a template, we can't run it without a type parameter.
// We speculatively run pe_main with X86_64, and if the speculation was
// wrong, re-run it with an actual machine type.
template <typename E>
int redo_main(Context<E> &ctx, int argc, char **argv) {
  //std::string_view target = ctx.arg.emulation;
  unreachable();
}

template <typename E>
void create_synthetic_sections(Context<E> &ctx) {
  auto push = [&](auto *x) {
    ctx.chunks.push_back(x);
    ctx.chunk_pool.emplace_back(x);
    return x;
  };

  // First chunk: MSDOS Stub
  if (!ctx.arg.stub.empty())
    ctx.msdos = push(new OutputMSDOS<E>(ctx, ctx.arg.stub));
  else
    ctx.msdos = push(new OutputMSDOS<E>());

  ctx.pe = push(new OutputPEHeader<E>());
}

// Copy chunks to an output file
template <typename E>
void copy_chunks(Context<E> &ctx) {
  Timer t(ctx, "copy_chunks");

  auto copy = [&](Chunk<E> &chunk) {
    std::string name = chunk.name.empty() ? "(header)" : std::string(chunk.name);
    Timer t2(ctx, name, &t);
    chunk.copy_buf(ctx);
  };

  tbb::parallel_for_each(ctx.chunks, [&](Chunk<E> *chunk) {
      copy(*chunk);
  });
}

template <typename E>
i64 set_osec_offsets(Context<E> & ctx)
{
  Timer t(ctx, "set_osec_offsets");

  std::vector<Chunk<E> *> &chunks = ctx.chunks;
  u64 fileoff = 0;
  i64 i = 0;

  while (i < chunks.size()) {
    Chunk<E> &first = *chunks[i];

    fileoff += first.size();
    i++;
  }

  return fileoff;
}

template <typename E>
void compute_section_headers(Context<E> &ctx) {
  // Update data for each chunk.
  for (Chunk<E> *chunk : ctx.chunks)
    chunk->update_shdr(ctx);
}

template <typename E>
void compute_grouped_sections(Context<E> &ctx) {
  
}

using E = MOLD_TARGET;

template int redo_main(Context<E> &, int, char **);
template void create_synthetic_sections(Context<E> &);
template void copy_chunks(Context<E> &);
template i64 set_osec_offsets(Context<E> &);
template void compute_section_headers(Context<E> &);
template void compute_grouped_sections(Context<E> &);

} // namespace mold::pe
