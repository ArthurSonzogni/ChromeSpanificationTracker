#include <cmath>
#include <cstdlib>
#include <iostream>
#include "parser.hpp"
#include <ranges>
#include <algorithm>

// C++20

// For each pattern, we count the number of added/removed lines, in tests and
// non tests.
const auto patterns = std::to_array({
    "#pragma allow_unsafe_buffers",
    "UNSAFE_BUFFERS(",
    "UNSAFE_TODO(",
    "UNSAFE_BUFFERS_USAGE",
  });

struct Count {
  int added = 0;
  int removed = 0;
  int added_test = 0;
  int removed_test = 0;
};

bool is_empty(Count c) {
  return c.added == 0 && c.removed == 0 && c.added_test == 0 && c.removed_test == 0;
}

std::array<Count, patterns.size()> count(Commit commit) {
  std::array<Count, patterns.size()> result;

  for (auto file : GetFiles(commit)) {
    const bool is_test = file.right_file.find("test") != std::string::npos;
    for (auto hunk : file.hunks) {
      for (auto line : hunk.lines) {
        for (size_t i = 0; i < patterns.size(); ++i) {
          const auto& pattern = patterns[i];
          if (line.content.find(pattern) == std::string::npos) {
            continue;
          }

          switch (line.type) {
            case Line::Keep:
              break;
            case Line::Add:
              (is_test ? result[i].added_test : result[i].added)++;
              break;
            case Line::Delete:
              (is_test ? result[i].removed_test : result[i].removed)++;
              break;
          }
        }
      }
    }
  }

  // Normalize the count.
  for (auto& c : result) {
    int changed = std::min(c.added, c.removed);
    c.added -= changed;
    c.removed -= changed;

    int changed_test = std::min(c.added_test, c.removed_test);
    c.added_test -= changed_test;
    c.removed_test -= changed_test;
  }

  return result;
}

int main(int argument_count, char** argument_values) {
  const std::string last_hash = (argument_count > 1) ? argument_values[1] : "";
  (void)argument_count;
  (void)argument_values;

  auto head = ResolveHead();

  int i = 0;
  while (true) {
    ++i;
    auto commit = GetCommit(head);

    auto counts = count(commit);

    const bool all_counts_empty = std::ranges::all_of(counts, is_empty);
    if (!all_counts_empty) {
      std::cout << commit.timestamp << "\t"         //
                << commit.hash << "\t";       //
      for (const Count& c : counts) {
        std::cout << c.added << "\t"             //
                  << c.removed << "\t"           //
                  << c.added_test << "\t"        //
                  << c.removed_test << "\t";
      }
      std::cout << commit.authors[0] << "\t" //
                << commit.title << std::endl;
    }

    if (commit.parents.size() == 0)
      break;

    head = commit.parents[0];

    if (head == last_hash)
      break;
  }
  return EXIT_SUCCESS;
}
