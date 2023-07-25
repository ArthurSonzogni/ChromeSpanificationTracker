#include <cmath>
#include <cstdlib>
#include <iostream>
#include "parser.hpp"

int main(int argument_count, char** argument_values) {
  const std::string last_hash = (argument_count > 1) ? argument_values[1] : "";
  (void)argument_count;
  (void)argument_values;

  auto head = ResolveHead();

  int i = 0;
  while (true) {
    ++i;
    auto commit = GetCommit(head);
    int added = 0;
    int removed = 0;
    int added_test = 0;
    int removed_test = 0;
    for (auto file : GetFiles(commit)) {
      const bool is_test = file.right_file.find("test") != std::string::npos;
      for (auto hunk : file.hunks) {
        for (auto line : hunk.lines) {
          if (line.content.find("raw_") == std::string::npos) {
            continue;
          }

          if (line.content.find("DanglingUntriaged") == std::string::npos &&
              line.content.find("DanglingAcrossTasks") == std::string::npos) {
            continue;
          }

          switch (line.type) {
            case Line::Keep:
              break;
            case Line::Add:
              (is_test ? added_test : added)++;
              break;
            case Line::Delete:
              (is_test ? removed_test : removed)++;
              break;
          }
        }
      }
    }

    int changed = std::min(added, removed);
    added -= changed;
    removed -= changed;

    int changed_test = std::min(added_test, removed_test);
    added_test -= changed_test;
    removed_test -= changed_test;

    if (added + removed + added_test + removed_test != 0) {
      std::cout << commit.timestamp << "\t"         //
                << commit.hash << "\t"       //
                << added << "\t"             //
                << removed << "\t"           //
                << added_test << "\t"        //
                << removed_test << "\t"      //
                << commit.authors[0] << "\t" //
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
