#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct Status {
  int code = 0;        // 0 = OK
  std::string message; // if code != 0
};

struct Options {
  std::filesystem::path input;
  bool recursive = false;
  bool continue_on_error = false;
  std::vector<std::string> exts; // e.g. ".jsonl", ".csv"
  std::size_t preview_lines = 0;
};

struct Totals {
  std::uint64_t files = 0;
  std::uint64_t lines = 0;
  std::uint64_t bytes = 0;

  std::uint64_t errors = 0;
  std::vector<std::string> preview;
  std::vector<std::string> error_messages; // store a few, like 5
};

Status run(const Options &opt, Totals &totals);
