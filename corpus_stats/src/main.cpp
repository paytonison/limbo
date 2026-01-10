// corpus_stats.cpp (C++20)
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

struct Status {
  int code = 0;        // 0 = OK
  std::string message; // if code != 0
};

struct Options {
  fs::path input;
  bool recursive = false;
  std::vector<std::string> exts; // e.g. ".txt", ".jsonl"
  std::size_t preview_lines = 0;
  bool help = false;
  bool continue_on_error = false;
};

static Status Ok() { return {0, ""}; }

#define RETURN_IF_ERROR(expr)                                                  \
  do {                                                                         \
    Status _s = (expr);                                                        \
    if (_s.code != 0)                                                          \
      return _s;                                                               \
  } while (0)

static void print_usage(const char *prog) {
  std::cout << "Usage:\n"
            << "  " << prog
            << " <path> [--recursive|-r] [--ext .jsonl] [--ext .csv] "
               "[--preview N]\n\n"
            << "Examples:\n"
            << "  " << prog << " data/ -r --ext .jsonl --ext .csv --preview 5\n"
            << "  " << prog << " file.txt --preview 3\n";
}

static bool has_any_ext_filter(const Options &opt) { return !opt.exts.empty(); }

static bool ext_allowed(const fs::path &p, const Options &opt) {
  if (!has_any_ext_filter(opt))
    return true;
  const std::string ext = p.extension().string();
  for (const auto &allowed : opt.exts) {
    if (ext == allowed)
      return true;
  }
  return false;
}

static std::optional<std::size_t> parse_usize(std::string_view s) {
  if (s.empty())
    return std::nullopt;
  std::size_t value = 0;
  for (char c : s) {
    if (c < '0' || c > '9')
      return std::nullopt;
    const std::size_t digit = static_cast<std::size_t>(c - '0');
    // basic overflow guard
    if (value > (static_cast<std::size_t>(-1) - digit) / 10)
      return std::nullopt;
    value = value * 10 + digit;
  }
  return value;
}

static Status parse_args(int argc, char **argv, Options &opt) {
  if (argc <= 1) {
    opt.help = true;
    return {1, "No path provided."};
  }

  for (int i = 1; i < argc; ++i) {
    std::string_view a(argv[i]);

    if (a == "--continue" || a == "-c") {
      opt.continue_on_error = true;
      continue;
    }

    if (a == "--help" || a == "-h") {
      opt.help = true;
      return {0, ""};
    }

    if (a == "--recursive" || a == "-r") {
      opt.recursive = true;
      continue;
    }

    if (a == "--ext") {
      if (i + 1 >= argc)
        return {2, "--ext requires a value like .jsonl"};
      opt.exts.emplace_back(argv[++i]);
      continue;
    }

    if (a.rfind("--ext=", 0) == 0) {
      opt.exts.emplace_back(std::string(a.substr(6)));
      continue;
    }

    if (a == "--preview") {
      if (i + 1 >= argc)
        return {3, "--preview requires a number"};
      auto n = parse_usize(argv[++i]);
      if (!n)
        return {3, "--preview must be a non-negative integer"};
      opt.preview_lines = *n;
      continue;
    }

    if (a.rfind("--preview=", 0) == 0) {
      auto n = parse_usize(a.substr(10));
      if (!n)
        return {3, "--preview must be a non-negative integer"};
      opt.preview_lines = *n;
      continue;
    }

    // positional path
    if (opt.input.empty()) {
      opt.input = fs::path(a);
    } else {
      return {5, "Extra positional argument: " + std::string(a)};
    }
  }

  if (opt.input.empty())
    return {1, "No path provided."};
  return {0, ""};
}

struct Totals {
  std::uint64_t files = 0;
  std::uint64_t lines = 0;
  std::uint64_t bytes = 0;
  std::uint64_t errors = 0;
  std::vector<std::string> preview;
  std::vector<std::string> error_messages;
};

static Status scan_one_file(const fs::path &p, Totals &t,
                            std::size_t preview_lines) {
  std::ifstream in(p);
  if (!in)
    return {10, "Failed to open: " + p.string()};

  std::string line;
  std::uint64_t local_lines = 0;

  while (std::getline(in, line)) {
    ++local_lines;
    if (preview_lines > 0 && t.preview.size() < preview_lines) {
      t.preview.push_back(line);
    }
  }

  t.lines += local_lines;

  std::error_code ec;
  auto sz = fs::file_size(p, ec);
  if (!ec)
    t.bytes += static_cast<std::uint64_t>(sz);

  ++t.files;
  return {0, ""};
}

static Status run(const Options &opt, Totals &totals) {
  if (!fs::exists(opt.input))
    return {20, "Path does not exist: " + opt.input.string()};

  if (fs::is_regular_file(opt.input)) {
    if (!ext_allowed(opt.input, opt))
      return {0, ""}; // nothing to do
    return scan_one_file(opt.input, totals, opt.preview_lines);
  }

  if (!fs::is_directory(opt.input)) {
    return {21,
            "Path is neither a file nor a directory: " + opt.input.string()};
  }

  std::error_code ec;

  if (opt.recursive) {
    for (fs::recursive_directory_iterator it(opt.input, ec), end; it != end;
         it.increment(ec)) {
      if (ec)
        return {22, "Directory iteration error: " + ec.message()};
      const auto &p = it->path();
      if (!fs::is_regular_file(p))
        continue;
      if (!ext_allowed(p, opt))
        continue;
      Status s = scan_one_file(p, totals, opt.preview_lines);
      if (s.code != 0) {
        if (!opt.continue_on_error)
          return s;
        ++totals.errors;
        if (totals.error_messages.size() < 5) {
          totals.error_messages.push_back(s.message);
        }
        continue;
      }
    }
  } else {
    for (fs::directory_iterator it(opt.input, ec), end; it != end;
         it.increment(ec)) {
      if (ec)
        return {22, "Directory iteration error: " + ec.message()};
      const auto &p = it->path();
      if (!fs::is_regular_file(p))
        continue;
      if (!ext_allowed(p, opt))
        continue;
      Status s = scan_one_file(p, totals, opt.preview_lines);
      if (s.code != 0) {
        if (!opt.continue_on_error)
          return s;
        ++totals.errors;
        if (totals.error_messages.size() < 5) {
          totals.error_messages.push_back(s.message);
        }
        continue;
      }
    }
  }

  return {0, ""};
}

int main(int argc, char **argv) {
  Options opt;
  Status ps = parse_args(argc, argv, opt);

  if (opt.help || ps.code == 1) {
    print_usage(argv[0]);
    if (ps.code != 0 && !ps.message.empty()) {
      std::cerr << "\nError: " << ps.message << "\n";
    }
    return (ps.code == 0) ? 0 : ps.code;
  }

  if (ps.code != 0) {
    std::cerr << "Argument error: " << ps.message << "\n";
    return ps.code;
  }

  Totals totals;
  Status s = run(opt, totals);
  if (s.code != 0) {
    std::cerr << "Run error: " << s.message << "\n";
    return s.code;
  }

  std::cout << "Files: " << totals.files << "\n";
  std::cout << "Lines: " << totals.lines << "\n";
  std::cout << "Bytes: " << totals.bytes << "\n";

  if (!totals.preview.empty()) {
    std::cout << "\nPreview (" << totals.preview.size() << " lines):\n";
    for (std::size_t i = 0; i < totals.preview.size(); ++i) {
      std::cout << (i + 1) << ": " << totals.preview[i] << "\n";
    }
  }

  if (totals.errors > 0) {
    std::cout << "\nErrors: " << totals.errors << "\n";
    for (const auto &msg : totals.error_messages) {
      std::cout << "  " << msg << "\n";
    }
  }

  return 0;
}
