#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

struct Status {
  int code;            // 0 = OK
  std::string message; // human-readable error
};

struct ScanResult {
  std::uint64_t lines = 0;
  std::vector<std::string> preview;
};

// Exit codes (kept from your original scheme)
// 0 OK
// 2 Usage / bad args
// 3 Bad extension
// 4 Does not exist
// 5 Not a regular file
// 6 Open failure
// 7 Read failure

static void print_help(const char *exe) {
  std::cout
      << "Usage: " << exe << " [options] <path|-> [preview_lines]\n"
      << "\n"
      << "Options:\n"
      << "  -h, --help              Show this help\n"
      << "  -p, --preview N          Preview first N lines (default 5)\n"
      << "      --preview=N          Same as above\n"
      << "\n"
      << "Arguments:\n"
      << "  <path|->                 Input file (.jsonl or .csv), or '-' for "
         "stdin\n"
      << "  [preview_lines]          Back-compat positional preview count\n"
      << "\n"
      << "Exit codes:\n"
      << "  0 OK\n"
      << "  2 Usage / bad args\n"
      << "  3 File must be .jsonl or .csv\n"
      << "  4 File does not exist\n"
      << "  5 Not a regular file\n"
      << "  6 Failed to open file\n"
      << "  7 Read error\n";
}

static bool has_allowed_extension(const std::filesystem::path &p) {
  const auto ext = p.extension().string(); // includes the dot
  return ext == ".jsonl" || ext == ".csv";
}

static Status validate_input(const std::filesystem::path &p) {
  if (p.empty())
    return {2, "No path provided."};

  // Allow stdin via "-"
  if (p == "-")
    return {0, ""};

  if (!has_allowed_extension(p))
    return {3, "File must be .jsonl or .csv."};

  std::error_code ec;
  if (!std::filesystem::exists(p, ec))
    return {4, "File does not exist."};

  if (!std::filesystem::is_regular_file(p, ec))
    return {5, "Path is not a regular file."};

  return {0, ""};
}

static Status parse_u64(std::string_view s, std::uint64_t &out) {
  if (s.empty())
    return {2, "Invalid number (empty)."};

  std::uint64_t v = 0;
  const char *b = s.data();
  const char *e = s.data() + s.size();
  auto [ptr, ec] = std::from_chars(b, e, v);

  if (ec != std::errc{} || ptr != e)
    return {2, "Invalid number. Must be a non-negative integer."};

  out = v;
  return {0, ""};
}

static Status scan_stream(std::istream &in, std::size_t k, ScanResult &out) {
  out = ScanResult{};
  out.preview.reserve(k);

  std::string line;
  while (std::getline(in, line)) {
    ++out.lines;
    if (out.preview.size() < k)
      out.preview.push_back(line);
  }

  if (in.bad())
    return {7, "Read error while scanning input."};

  return {0, ""};
}

static Status scan_file(const std::filesystem::path &path, std::size_t k,
                        ScanResult &out) {
  if (path == "-") {
    return scan_stream(std::cin, k, out);
  }

  std::ifstream f(path);
  if (!f)
    return {6, "Failed to open file for reading."};

  return scan_stream(f, k, out);
}

struct ParsedArgs {
  bool help = false;
  std::filesystem::path path;
  std::size_t preview_lines = 5;
};

static Status parse_args(int argc, char **argv, ParsedArgs &out) {
  out = ParsedArgs{};

  bool saw_positional_preview = false;
  int positionals = 0;

  for (int i = 1; i < argc; ++i) {
    std::string_view a = argv[i];

    if (a == "-h" || a == "--help") {
      out.help = true;
      return {0, ""};
    }

    if (a == "-p" || a == "--preview") {
      if (i + 1 >= argc)
        return {2, "Missing value for --preview. Run --help."};

      std::uint64_t v = 0;
      Status st = parse_u64(argv[++i], v);
      if (st.code != 0)
        return {2, "Invalid preview_lines. Must be a non-negative integer."};

      if (v >
          static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        return {2, "preview_lines is too large for this platform."};

      out.preview_lines = static_cast<std::size_t>(v);
      continue;
    }

    if (a.rfind("--preview=", 0) == 0) {
      std::string_view val = a.substr(std::string_view("--preview=").size());

      std::uint64_t v = 0;
      Status st = parse_u64(val, v);
      if (st.code != 0)
        return {2, "Invalid preview_lines. Must be a non-negative integer."};

      if (v >
          static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        return {2, "preview_lines is too large for this platform."};

      out.preview_lines = static_cast<std::size_t>(v);
      continue;
    }

    if (!a.empty() && a[0] == '-') {
      return {2, std::string("Unknown option: ") + std::string(a) +
                     ". Run --help."};
    }

    // Positional args:
    // 1) path
    // 2) optional preview_lines (back-compat)
    ++positionals;
    if (positionals == 1) {
      out.path = std::filesystem::path(std::string(a));
      continue;
    }

    if (positionals == 2) {
      if (saw_positional_preview)
        return {2, "Too many arguments. Run --help."};

      std::uint64_t v = 0;
      Status st = parse_u64(a, v);
      if (st.code != 0)
        return {2, "Invalid preview_lines. Must be a non-negative integer."};

      if (v >
          static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        return {2, "preview_lines is too large for this platform."};

      out.preview_lines = static_cast<std::size_t>(v);
      saw_positional_preview = true;
      continue;
    }

    return {2, "Too many arguments. Run --help."};
  }

  if (out.path.empty())
    return {2, "Missing input path. Run --help."};

  return {0, ""};
}

int main(int argc, char **argv) {
  ParsedArgs args;
  Status ast = parse_args(argc, argv, args);

  if (ast.code != 0) {
    std::cerr << "Error: " << ast.message << "\n";
    std::cerr << "Run --help for usage.\n";
    return ast.code;
  }

  if (args.help) {
    print_help(argv[0]);
    return 0;
  }

  const Status vst = validate_input(args.path);
  if (vst.code != 0) {
    std::cerr << "Error: " << vst.message << "\n";
    return vst.code;
  }

  ScanResult r;
  const Status sst = scan_file(args.path, args.preview_lines, r);
  if (sst.code != 0) {
    std::cerr << "Error: " << sst.message << "\n";
    return sst.code;
  }

  std::cout << "Lines: " << r.lines << "\n";
  std::cout << "Preview (" << r.preview.size() << "):\n";
  for (std::size_t i = 0; i < r.preview.size(); ++i) {
    std::cout << (i + 1) << ": " << r.preview[i] << "\n";
  }

  return 0;
}
