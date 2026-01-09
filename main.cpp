#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
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

static bool has_allowed_extension(const std::filesystem::path &p) {
  const auto ext = p.extension().string(); // includes the dot
  return ext == ".jsonl" || ext == ".csv";
}

static Status validate_input(const std::filesystem::path &p) {
  if (p.empty())
    return {2, "No path provided."};
  if (!has_allowed_extension(p))
    return {3, "File must be .jsonl or .csv."};
  if (!std::filesystem::exists(p))
    return {4, "File does not exist."};
  if (!std::filesystem::is_regular_file(p))
    return {5, "Path is not a regular file."};
  return {0, ""};
}

static Status scan_file(const std::filesystem::path &path, std::size_t k,
                        ScanResult &out) {
  std::ifstream f(path);
  if (!f)
    return {6, "Failed to open file for reading."};

  out = ScanResult{};
  out.preview.reserve(k);

  std::string line;
  while (std::getline(f, line)) {
    ++out.lines;
    if (out.preview.size() < k)
      out.preview.push_back(line);
  }

  return {0, ""};
}

int main(int argc, char **argv) {
  using std::string_view;
  if (argc < 2) {
    std::cerr << "Usage: inspect <path> [preview_lines]\n";
    return 2;
  }

  std::filesystem::path path = argv[1];
  std::size_t k = 5;

  if (argc >= 3) {
    try {
      k = static_cast<std::size_t>(std::stoul(argv[2]));
    } catch (...) {
      std::cerr << "Invalid preview_lines. Must be a non-negative integer.\n";
      return 2;
    }
  }

  const string_view a1 = argv[1];
  if (a1 == "--help" || a1 == "-h") {
    std::cout << "There is no help for you\n";
    return 0;
  }

  const Status vst = validate_input(path);
  if (vst.code != 0) {
    std::cerr << "Error: " << vst.message << "\n";
    return vst.code;
  }

  ScanResult r;
  const Status sst = scan_file(path, k, r);
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
