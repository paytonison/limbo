#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Options {
  fs::path input;
  bool recursive = false;
  std::vector<std::string> exts; // e.g. ".txt", ".jsonl"
  std::size_t preview_lines = 0;
  bool help = false;
  bool continue_on_error = false;
};
