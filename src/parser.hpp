// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#ifndef CHROME_SPANIFICATION_TRACKER_PARSER_HPP
#define CHROME_SPANIFICATION_TRACKER_PARSER_HPP

#include <array>
#include <functional>
#include <string>
#include <string_view>

inline constexpr auto patterns = std::to_array<std::string_view>({
    "#pragma allow_unsafe_buffers",
    "#pragma allow_unsafe_libc_calls",
    "UNSAFE_BUFFERS(",
    "UNSAFE_TODO(",
    "UNSAFE_BUFFER_USAGE",
});

struct Count {
  int added = 0;
  int removed = 0;
  int added_test = 0;
  int removed_test = 0;

  constexpr bool is_empty() const {
    return added == 0 && removed == 0 && added_test == 0 && removed_test == 0;
  }
};

struct CommitReport {
  std::string hash;
  std::string timestamp;
  std::string author;
  std::string title;
  std::array<Count, patterns.size()> counts{};
};

std::string BuildRegexPattern();

void TrackSpanification(
    std::string_view last_hash,
    const std::function<void(const CommitReport&)>& on_commit);

#endif  // CHROME_SPANIFICATION_TRACKER_PARSER_HPP
