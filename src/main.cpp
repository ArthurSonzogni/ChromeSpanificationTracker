#include <cmath>
#include <cstdlib>
#include <iostream>
#include "parser.hpp"

int main(int argument_count, char** argument_values) {
  (void)argument_count;
  (void)argument_values;

  auto head = ResolveHead();

  int i = 0;
  while (true) {
    ++i;
    auto commit = GetCommit(head);
    int added = 0;
    int removed = 0;
    for (auto file : GetFiles(commit)) {
      for (auto hunk : file.hunks) {
        for (auto line : hunk.lines) {
          if (line.content.find("raw_") == std::string::npos ||
              line.content.find(", DanglingUntriaged") == std::string::npos) {
            continue;
          }
          switch (line.type) {
            case Line::Keep:
              break;
            case Line::Add:
              added++;
              break;
            case Line::Delete:
              removed++;
              break;
          }
        }
      }
    }

    int changed = std::max(added, removed);
    added -= changed;
    removed -= changed;

    if (added + removed != 0) {
      std::cout << i << "\t"                  //
                << commit.hash << "\t"        //
                << added << "\t"              //
                << removed << "\t"            //
                << commit.authors[0] << "\t"  //
                << commit.title << std::endl;
    }

    if (commit.parents.size() == 0)
      break;
    head = commit.parents[0];
  }
  std::cout << head << std::endl;
  return EXIT_SUCCESS;
}
