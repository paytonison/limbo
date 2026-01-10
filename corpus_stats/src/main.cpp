#include "corpus_stats.hpp"

#include <iostream>
#include <optional>
#include <string_view>

static void print_usage(const char *prog) {
  std::cout << "Usage:\n"
            << "  " << prog
            << " <path> [--recursive|-r] [--continue|-c] [--ext .jsonl] [--ext "
               ".csv] [--preview N]\n\n"
            << "Examples:\n"
            << "  " << prog << " data/ -r --ext .jsonl --ext .csv --preview 5\n"
            << "  " << prog << " data/ -r -c --ext .jsonl\n"
            << "  " << prog << " file.txt --preview 3\n";
}

static std::optional<std::size_t> parse_usize(std::string_view s) {
  if (s.empty())
    return std::nullopt;
  std::size_t value = 0;
  for (char c : s) {
    if (c < '0' || c > '9')
      return std::nullopt;
    const std::size_t digit = static_cast<std::size_t>(c - '0');
    if (value > (static_cast<std::size_t>(-1) - digit) / 10)
      return std::nullopt;
    value = value * 10 + digit;
  }
  return value;
}

static Status parse_args(int argc, char **argv, Options &opt, bool &help) {
  help = false;

  if (argc <= 1) {
    help = true;
    return {1, "No path provided."};
  }

  for (int i = 1; i < argc; ++i) {
    std::string_view a(argv[i]);

    if (a == "--help" || a == "-h") {
      help = true;
      return {0, ""};
    }

    if (a == "--recursive" || a == "-r") {
      opt.recursive = true;
      continue;
    }

    if (a == "--continue" || a == "-c") {
      opt.continue_on_error = true;
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

    if (!a.empty() && a[0] == '-') {
      return {4, "Unknown option: " + std::string(a)};
    }

    if (opt.input.empty()) {
      opt.input = std::filesystem::path(a);
    } else {
      return {5, "Extra positional argument: " + std::string(a)};
    }
  }

  if (opt.input.empty())
    return {1, "No path provided."};
  return {0, ""};
}

int main(int argc, char **argv) {
  Options opt;
  bool help = false;

  Status ps = parse_args(argc, argv, opt, help);
  if (help || ps.code == 1) {
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

  if (totals.errors > 0) {
    std::cout << "Errors: " << totals.errors << "\n";
    for (const auto &msg : totals.error_messages) {
      std::cout << "  - " << msg << "\n";
    }
  }

  if (!totals.preview.empty()) {
    std::cout << "\nPreview (" << totals.preview.size() << " lines):\n";
    for (std::size_t i = 0; i < totals.preview.size(); ++i) {
      std::cout << (i + 1) << ": " << totals.preview[i] << "\n";
    }
  }

  return 0;
}
