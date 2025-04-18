#include "mold.h"
#include "../common/archive-file.h"
#include "../common/output-file.h"

#include <tbb/global_control.h>
#include <tbb/parallel_for_each.h>

#ifdef _WIN32
# include <direct.h>
# define chdir _chdir
#else
# include <unistd.h>
#endif

#ifdef MOLD_X86_64
int main(int argc, char **argv) {
  return mold::pe::pe_main<mold::pe::X86_64>(argc, argv);
}
#endif

namespace mold::pe {


// Read the beginning of a given file and returns its machine type
// (e.g. IMAGE_FILE_MACHINE_AMD64).
template <typename E>
std::string_view get_machine_type(Context<E> &ctx, MappedFile *mf) {

  auto get_arch_name = [&](u16 *buf) -> std::string_view {
    switch (*buf) {
    case IMAGE_FILE_MACHINE_AMD64:
      return X86_64::target_name;
    default:
      return "";
    }
  };

  switch (get_file_type(ctx, mf)) {
  case FileType::PE_OBJ:
    return get_arch_name((u16*)mf->data);
  default:
    return "";
  }
}

template <typename E>
static void
check_file_compatibility(Context<E> &ctx, MappedFile *mf) {

  std::string_view target = get_machine_type(ctx, mf);
  if (target != ctx.arg.emulation)
    Fatal(ctx) << mf->name << ": incompatible file type: "
               << ctx.arg.emulation << " is expected but got " << target;
}

template <typename E>
static std::string_view
detect_machine_type(Context<E> &ctx, std::vector<std::string> paths) {
  std::erase(paths, "-");

  for (const std::string &path : paths)
    if (auto *mf = open_file(ctx, path))
        if (std::string_view target = get_machine_type(ctx, mf);
            !target.empty())
          return target;
  
  Fatal(ctx) << "/ARCH option is missing";
}

template <typename E>
static ObjectFile<E> *new_object_file(Context<E> &ctx, MappedFile *mf,
                                      std::string archive_name) {
  static Counter count("parsed_objs");
  count++;

  check_file_compatibility(ctx, mf);

  ObjectFile<E> *file = ObjectFile<E>::create(ctx, mf, archive_name, false);
  file->priority = ctx.file_priority++;
  ctx.tg.run([file, &ctx] { file->parse(ctx); });
  if (ctx.arg.trace)
    SyncOut(ctx) << "trace: " << *file;
  return file;
}

template <typename E>
void read_file(Context<E> &ctx, MappedFile *mf) {
  if (ctx.visited.contains(mf->name))
    return;

  switch (get_file_type(ctx, mf)) {
  case FileType::PE_OBJ:
    ctx.objs.push_back(new_object_file(ctx, mf, ""));
    return;
  default:
    Fatal(ctx) << mf->name << ": unknown file type";
  }
}

template <typename E>
static void read_input_files(Context<E> &ctx, std::span<std::string> args) {
  Timer t(ctx, "read_input_files");

  while (!args.empty()) {
    std::string_view arg = args[0];
    args = args.subspan(1);
    read_file(ctx, must_open_file(ctx, std::string(arg)));
  }

  if (ctx.objs.empty())
    Fatal(ctx) << "no input files";

  ctx.tg.wait();
}

template <typename E>
int pe_main(int argc, char **argv) {
    Context<E> ctx;

  // Parse non-positional command line options
  ctx.cmdline_args = pack_args(ctx, argv);
  std::vector<std::string> file_args = parse_nonpositional_args(ctx);

  // If no -m option is given, deduce it from input files.
  if (ctx.arg.emulation.empty())
    ctx.arg.emulation = detect_machine_type(ctx, file_args);

  Timer t_all(ctx, "all");

  install_signal_handler();

  // Fork a subprocess unless --no-fork is given.
  std::function<void()> on_complete;

#if !defined(_WIN32) && !defined(__APPLE__)
  if (ctx.arg.fork)
    on_complete = fork_child();
#endif

  acquire_global_lock();


  tbb::global_control tbb_cont(tbb::global_control::max_allowed_parallelism,
                               ctx.arg.thread_count);

  // Parse input files
  read_input_files(ctx, file_args);


  Timer t_before_copy(ctx, "before_copy");
  
  // TODO: support symbol resolving
  // TODO: here we should also support /OPT:ICF (ctx.arg.icf + icf_sections from PE)
  // TODO: here we should also support /OPT:REF

  // We need to merge the various text$xyxy sections and sort them
  compute_grouped_sections(ctx);

  // Create linker-synthesized sections and header such as the MSDOS stub
  create_synthetic_sections(ctx);

  // Compute the section header values for all sections.
  compute_section_headers(ctx);

  // Assign offsets to output sections
  i64 filesize = set_osec_offsets(ctx);
  
  // At this point, both memory and file layouts are fixed.

  t_before_copy.stop();

  // Create an output file
  ctx.output_file =
    OutputFile<Context<E>>::open(ctx, ctx.arg.output, filesize, 0777);
  ctx.buf = ctx.output_file->buf;

  Timer t_copy(ctx, "copy");

  // Copy input sections to the output file and apply relocations.
  copy_chunks(ctx);

  t_copy.stop();
  ctx.checkpoint();

  // Close the output file. This is the end of the linker's main job.
  ctx.output_file->close(ctx);

  t_all.stop();


  std::cout << std::flush;
  std::cerr << std::flush;

  if (on_complete)
    on_complete();

  release_global_lock();


  if (ctx.arg.quick_exit)
    _exit(0);

  for (std::function<void()> &fn : ctx.on_exit)
    fn();
  ctx.checkpoint();
  return 0;
}

using E = MOLD_TARGET;

template int pe_main<E>(int, char **);

}
