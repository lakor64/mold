#include "mold.h"

#include <tbb/global_control.h>

#ifdef _WIN32
# define isatty _isatty
# define STDERR_FILENO (_fileno(stderr))
#else
# include <unistd.h>
#endif

namespace mold::pe {

inline const char helpmsg[] = R"(
Options:
  /?                                 Report usage information
  /base:ADDRESS                      Specifies the base address of the file
  /dll                               Builds a shared library and not an executable
  /fork[:no]                         Spawn a child process (default)
  /ignore:all                        Ignores every warning emitted by the linker
  /largeaddressaware[:no]            Tells the linker that the application can handle addresses larger
                                      than 2GB (default on 64-bit platforms) 
  /libpath:DIR                       Add DIR to library search path
  /machine:TARGET                    Set target
  /nologo                            Does not print the version information at invoke
  /out:TARGET                        Set output filename
  /quickexit[:no]                    Enable or disable quick_exit to exit (default)
  /stub:FILE                         Attaches a different MS-DOS stub program rather than the default one
  /subsystem:SUBSYS[,MAJOR.MINOR]    Specifies the NT subsystem targeted by the executable and optionally 
                                      the minimum version required to run it (default is CONSOLE,6.0)
  /threadcount:COUNT                 Use COUNT number of threads
  /verbose                           Prints trace messages
  /version:MAJOR[.MINOR]             Sets the Image version of the target executable
  /wx[:no]                           Treat warnings as errors

mold: supported machine targets: X64
)";


static std::vector<std::string> add_dashes(std::string name) {
  return {"-" + name, "/" + name};
}

static i64 get_default_thread_count() {
  // mold doesn't scale well above 32 threads.
  int n = tbb::global_control::active_value(
    tbb::global_control::max_allowed_parallelism);
  return std::min(n, 32);
}

template <typename E>
std::vector<std::string_view>
pack_args(Context<E> &ctx, char **argv) {
  std::vector<std::string_view> vec;
  for (i64 i = 0; argv[i]; i++) {
      vec.push_back(argv[i]);
  }
  return vec;
}

template <typename E>
static i64 parse_number(Context<E> &ctx, std::string opt,
                        std::string_view value) {
  size_t nread;

  if (value.starts_with('-')) {
    i64 ret = std::stoul(std::string(value.substr(1)), &nread, 0);
    if (value.size() - 1 != nread)
      Fatal(ctx) << "option /" << opt << ": not a number: " << value;
    return -ret;
  }

  i64 ret = std::stoul(std::string(value), &nread, 0);
  if (value.size() != nread)
    Fatal(ctx) << "option /" << opt << ": not a number: " << value;
  return ret;
}


template <typename E>
std::vector<std::string> parse_nonpositional_args(Context<E> &ctx) {
  std::span<std::string_view> args = ctx.cmdline_args;
  args = args.subspan(1);

  std::vector<std::string> remaining;
  std::string_view arg;
  std::string arglow; // avoid killing arg view point
  bool version_shown = true;

  ctx.arg.color_diagnostics = isatty(STDERR_FILENO);

  auto read_flag = [&](std::string name) {
    // msvc link flags are expected to be case insensitive
    arglow = std::string(args[0]);
    std::transform(arglow.begin(), arglow.end(), arglow.begin(), ::tolower);
    for (const std::string &opt : add_dashes(name)) {
      if (arglow == opt) {
        args = args.subspan(1);
        return true;
      }
    }
    return false;
  };

  auto read_arg = [&](std::string name, bool arg_value_case_ins = true) {
    // msvc link flags are expected to be case insensitive
    arglow = std::string(args[0]);
    auto it = arglow.end();
    const auto p = arglow.find(":");
    if (p != std::string::npos)
    {
      it = arglow.begin() + p;
    }

    std::transform(arglow.begin(), it, arglow.begin(), ::tolower);

    for (const std::string &opt : add_dashes(name)) {
      std::string prefix = (p == std::string::npos) ? opt : opt + ":";
      if (arglow.starts_with(prefix)) {
        arglow = arglow.substr(prefix.size());
        args = args.subspan(1);
        if (arg_value_case_ins)
          std::transform(arglow.begin(), arglow.end(), arglow.begin(), ::tolower);
        arg = arglow;
        return true;
      }
    }
    return false;
  };

  auto read_yesno = [&](std::string name) {
      if (!read_arg(name, true))
          return false;

      if (arg.empty())
          arg = "yes";

      if (arg != "yes" && arg != "no")
          Fatal(ctx) << "option /" << name << ": invalid value " << arg;

      return true;
      };

  std::vector<std::string_view> args2;

  // high priority arguments
  while (!args.empty()) {
      if (read_flag("?")) {
          SyncOut(ctx) << get_mold_version() << "\n"
              << "Usage: " << ctx.cmdline_args[0]
              << " [options] file...\n" << helpmsg;
          exit(0);
      } else if (read_flag("nologo")) {
          version_shown = false;
      } else if (read_flag("verbose")) {
          ctx.arg.trace = true;
      } else if (read_yesno("quickexit")) {
          ctx.arg.quick_exit = arg != "no";
      } else if (read_yesno("wx")) {
          ctx.arg.fatal_warnings = arg != "no";
      } else {
          args2.emplace_back(args[0]);
          args = args.subspan(1);
      }
  }

  if (version_shown)
      SyncOut(ctx) << get_mold_version();
    
  if constexpr (!E::is_64)
    ctx.arg.large_address = false; // large address aware is false on 32-bit by default

  args = args2;

  while (!args.empty()) {
    if (read_yesno("fork")) {
      ctx.arg.fork = arg != "no";
    } else if (read_arg("machine")) {
      if (arg == "amd64" || arg == "x64") {
        ctx.arg.emulation = X86_64::target_name;
      } else {
        Fatal(ctx) << "unknown machine argument: " << arg;
      }
    } else if (read_arg("threadcount", false)) {
      ctx.arg.thread_count = parse_number(ctx, "threadcount", arg);
    } else if (read_arg("out", false)) {
      ctx.arg.output = arg;
    } else if (read_arg("libpath", false)) {
      ctx.arg.library_paths.push_back(std::string(arg));
    } else if (read_arg("largeaddressaware")) {
      ctx.arg.large_address = arg != "no";
    } else if (read_arg("stub", false)) {
      ctx.arg.stub = std::string(arg);
    } else if (read_flag("dll")) {
      ctx.arg.dll = true;
    } else if (read_arg("base")) {
      ctx.arg.base = parse_number(ctx, "base", arg);
      if (ctx.arg.base % 0x10000)
        Fatal(ctx) << "specified base is not a multiple of 64K";
    } else if (read_arg("ignore", false))
    {
      // NOTE: this is not MSVC compatible, but we want to avoid CLI errors
      //  if we pass /ignore:4000 for example
      if (arg == "all")
        ctx.arg.suppress_warnings = true;
    } else if (read_arg("version")) {
      auto pos = arg.find(".");
      if (pos == arg.npos || pos == arg.size() - 1)
      {
        ctx.arg.image_major_version = parse_number(ctx, "version", arg);
      }
      else
      {
        ctx.arg.image_major_version = parse_number(ctx, "version", arg.substr(0, pos));
        ctx.arg.image_minor_version = parse_number(ctx, "version", arg.substr(pos + 1));
      }

      if (ctx.arg.image_minor_version < 0 || ctx.arg.image_minor_version > 0xffff)
          Fatal(ctx) << "invalid minor image version specified";
      if (ctx.arg.image_major_version < 0 || ctx.arg.image_major_version > 0xffff)
          Fatal(ctx) << "invalid major image version specified";
    } else if (read_arg("subsystem")) {
      std::string_view subsys_name;
      auto pos2 = arg.find(",");
      bool managed = false;
      if (pos2 != arg.npos && pos2 != arg.size() - 1)
      {
        subsys_name = arg.substr(0, pos2);
      }
      else
        subsys_name = arg;

      if (subsys_name == "windows")
      {
        // NOTE: this changes if arm
        ctx.arg.subsystem_major_version = 6;
        ctx.arg.subsystem_minor_version = 0;
        ctx.arg.subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
      }
      else if (subsys_name == "posix")
      {
        ctx.arg.subsystem_major_version = 19;
        ctx.arg.subsystem_minor_version = 90;
        ctx.arg.subsystem = IMAGE_SUBSYSTEM_POSIX_CUI;
      }
      else if (subsys_name == "native")
      {
        // NOTE: this changes if x86/arm
        ctx.arg.subsystem_major_version = 5;
        ctx.arg.subsystem_minor_version = 2;
        ctx.arg.subsystem = IMAGE_SUBSYSTEM_NATIVE;
      }
      else if (subsys_name != "console")
        Fatal(ctx) << "invalid subsystem: " << subsys_name;

      if (pos2 != arg.npos && pos2 != arg.size() - 1)
      {
        auto pos = arg.find(".", pos2);
        if (pos == arg.npos || pos == arg.size() - 1)
        {
          ctx.arg.subsystem_major_version = parse_number(ctx, "subsystem", arg.substr(pos2 + 1));
          ctx.arg.subsystem_minor_version = 0;
        }
        else
        {
          ctx.arg.subsystem_major_version = parse_number(ctx, "subsystem", arg.substr(pos2 + 1, pos - pos2 - 1));
          ctx.arg.subsystem_minor_version = parse_number(ctx, "subsystem", arg.substr(pos + 1));
        }

        if (ctx.arg.subsystem_minor_version < 0 || ctx.arg.subsystem_minor_version > 0xffff)
            Fatal(ctx) << "invalid minor subsystem version specified";
        if (ctx.arg.subsystem_major_version < 0 || ctx.arg.subsystem_major_version > 0xffff)
            Fatal(ctx) << "invalid major subsystem version specified";
        
        // Checks to avoid targetting a NT subsystem that does not support the machine
        // NOTE: This changes if x86/arm/whatever
        if ((ctx.arg.subsystem == IMAGE_SUBSYSTEM_NATIVE ||
            ctx.arg.subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI ||
            ctx.arg.subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) &&
            (ctx.arg.subsystem_major_version < 5 || // NT4 does not support x64
                ctx.arg.subsystem_major_version == 5 && ctx.arg.subsystem_minor_version < 2)) // x64 since XP
        {
            Warn(ctx) << "the specified subsystem version is not supported for the target machine. The default subsystem version will be used instead";
            ctx.arg.subsystem_major_version = 5;
            ctx.arg.subsystem_minor_version = 2;
        }
      }
    } else {
      if (args[0][0] == '-' || args[0][0] == '/')
        Fatal(ctx) << "unknown command line option: " << args[0];
      remaining.push_back(std::string(args[0]));
      args = args.subspan(1);
    }
  }

  // Setup default base address values
  if (ctx.arg.base == -1)
  {
    if (ctx.arg.dll)
    {
      if constexpr (E::is_64)
          ctx.arg.base = 0x180000000;
      else if constexpr (!E::is_64)
          ctx.arg.base = 0x10000000;
    }
    else if constexpr (E::is_64)
        ctx.arg.base = 0x140000000;
    else if constexpr (!E::is_64)
        ctx.arg.base = 0x400000;
  }

  // Clean library paths by removing redundant `/..` and `/.`
  // so that they are easier to read in log messages.
  for (std::string &path : ctx.arg.library_paths)
    path = path_clean(path);

  if (ctx.arg.thread_count == 0)
    ctx.arg.thread_count = get_default_thread_count();

  return remaining;
}

using E = MOLD_TARGET;

template std::vector<std::string_view> pack_args(Context<E> &, char **);
template std::vector<std::string> parse_nonpositional_args(Context<E> &ctx);

} // namespace mold::pe
