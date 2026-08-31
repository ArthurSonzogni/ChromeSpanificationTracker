// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include "parser.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "subprocess/ProcessBuilder.hpp"
#include "subprocess/basic_types.hpp"
#include "subprocess/pipe.hpp"

std::string BuildRegexPattern() {
  std::string result;
  result += "(";
  for (size_t i = 0; i < patterns.size(); ++i) {
    if (i > 0) {
      result += "|";
    }
    for (char c : patterns[i]) {
      switch (c) {
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '\\':
        case '^':
        case '$':
        case '.':
        case '*':
        case '+':
        case '?':
        case '|':
          result += '\\';
          [[fallthrough]];
        default:
          result += c;
          break;
      }
    }
  }
  result += ")";
  return result;
}

namespace {

class StreamLineReader {
 public:
  explicit StreamLineReader(subprocess::PipeHandle handle) : handle_(handle) {
    buffer_.reserve(64 * 1024);
  }

  bool ReadLine(std::string_view& line) {
    while (true) {
      size_t newline_pos = buffer_.find('\n', pos_);
      if (newline_pos != std::string::npos) {
        line = std::string_view(buffer_.data() + pos_, newline_pos - pos_);
        pos_ = newline_pos + 1;
        return true;
      }

      if (pos_ > 0) {
        buffer_.erase(0, pos_);
        pos_ = 0;
      }

      char read_chunk[64 * 1024];
      subprocess::ssize_t bytes_read =
          subprocess::pipe_read(handle_, read_chunk, sizeof(read_chunk));
      if (bytes_read <= 0) {
        if (pos_ < buffer_.size()) {
          line = std::string_view(buffer_.data() + pos_, buffer_.size() - pos_);
          pos_ = buffer_.size();
          return true;
        }
        return false;
      }

      buffer_.append(read_chunk, static_cast<size_t>(bytes_read));
    }
  }

 private:
  subprocess::PipeHandle handle_;
  std::string buffer_;
  size_t pos_ = 0;
};

}  // namespace

void TrackSpanification(
    std::string_view last_hash,
    const std::function<void(const CommitReport&)>& on_commit) {
  std::string range;
  if (!last_hash.empty() && last_hash != "empty") {
    range = std::string(last_hash) + "..HEAD";
  } else {
    auto check_rev = subprocess::run(
        {"git", "rev-parse", "--verify", "-q",
         "59f56d97fff01d8fed1de28f4a7865d62f80ad1a~1"},
        subprocess::RunBuilder()
            .cerr(subprocess::PipeOption::pipe)
            .cout(subprocess::PipeOption::pipe)
            .cin(subprocess::PipeOption::close));
    if (check_rev.returncode == 0) {
      range = "59f56d97fff01d8fed1de28f4a7865d62f80ad1a~1..HEAD";
    } else {
      range = "HEAD";
    }
  }
  std::string regex = BuildRegexPattern();
  std::string format = "--format=\x1e%H\x1f%at\x1f%al\x1f%s";

  auto proc = subprocess::RunBuilder({
                                         "git",
                                         "log",
                                         "--first-parent",
                                         "--no-renames",
                                         "-p",
                                         "-U1",
                                         "-G",
                                         regex,
                                         format,
                                         range,
                                     })
                  .cout(subprocess::PipeOption::pipe)
                  .cin(subprocess::PipeOption::close)
                  .cerr(subprocess::PipeOption::inherit)
                  .popen();

  StreamLineReader reader(proc.cout);
  std::string_view line;

  CommitReport current_commit;
  bool in_commit = false;
  bool is_test = false;

  auto flush_commit = [&]() -> bool {
    if (!in_commit) {
      return true;
    }
    in_commit = false;

    // Normalize the count.
    for (auto& c : current_commit.counts) {
      int changed = std::min(c.added, c.removed);
      c.added -= changed;
      c.removed -= changed;

      int changed_test = std::min(c.added_test, c.removed_test);
      c.added_test -= changed_test;
      c.removed_test -= changed_test;
    }

    bool all_empty = true;
    for (const auto& c : current_commit.counts) {
      if (!c.is_empty()) {
        all_empty = false;
        break;
      }
    }

    if (!all_empty) {
      on_commit(current_commit);
    }

    if (current_commit.hash == "59f56d97fff01d8fed1de28f4a7865d62f80ad1a") {
      return false;
    }

    return true;
  };

  while (reader.ReadLine(line)) {
    if (line.starts_with('\x1e')) {
      if (!flush_commit()) {
        break;
      }

      in_commit = true;
      is_test = false;
      current_commit = CommitReport{};

      std::string_view header = line.substr(1);
      auto split_field = [](std::string_view& rest) -> std::string_view {
        size_t pos = rest.find('\x1f');
        if (pos == std::string_view::npos) {
          std::string_view field = rest;
          rest = {};
          return field;
        }
        std::string_view field = rest.substr(0, pos);
        rest = rest.substr(pos + 1);
        return field;
      };

      current_commit.hash = std::string(split_field(header));
      current_commit.timestamp = std::string(split_field(header));
      current_commit.author = std::string(split_field(header));
      current_commit.title = std::string(header);
      continue;
    }

    if (!in_commit) {
      continue;
    }

    if (line.starts_with("+++ ")) {
      std::string_view path = line.substr(4);
      if (path != "/dev/null") {
        is_test = (path.find("test") != std::string_view::npos);
      }
      continue;
    }

    if (line.starts_with("--- ")) {
      std::string_view path = line.substr(4);
      if (path != "/dev/null") {
        is_test = (path.find("test") != std::string_view::npos);
      }
      continue;
    }

    if (line.starts_with('+')) {
      std::string_view content = line.substr(1);
      for (size_t i = 0; i < patterns.size(); ++i) {
        if (content.find(patterns[i]) != std::string_view::npos) {
          if (is_test) {
            current_commit.counts[i].added_test++;
          } else {
            current_commit.counts[i].added++;
          }
        }
      }
      continue;
    }

    if (line.starts_with('-')) {
      std::string_view content = line.substr(1);
      for (size_t i = 0; i < patterns.size(); ++i) {
        if (content.find(patterns[i]) != std::string_view::npos) {
          if (is_test) {
            current_commit.counts[i].removed_test++;
          } else {
            current_commit.counts[i].removed++;
          }
        }
      }
      continue;
    }
  }

  flush_commit();
  proc.close();
}
