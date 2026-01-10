#include "corpus_stats.hpp"

#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

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

static void record_error(Totals &totals, const Status &s) {
  ++totals.errors;
  if (totals.error_messages.size() < 5) {
    totals.error_messages.push_back(s.message);
  }
}

Status run(const Options &opt, Totals &totals) {
  if (!fs::exists(opt.input))
    return {20, "Path does not exist: " + opt.input.string()};

  if (fs::is_regular_file(opt.input)) {
    if (!ext_allowed(opt.input, opt))
      return {0, ""};
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
        record_error(totals, s);
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
        record_error(totals, s);
        continue;
      }
    }
  }

  return {0, ""};
}
