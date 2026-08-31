#include <cstdlib>
#include <iostream>
#include <string_view>

#include "parser.hpp"

int main(int argument_count, char** argument_values) {
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  const std::string_view last_hash =
      (argument_count > 1) ? argument_values[1] : "";

  TrackSpanification(last_hash, [](const CommitReport& commit) {
    std::cout << commit.timestamp << "\t"  //
              << commit.hash << "\t";      //
    for (const auto& c : commit.counts) {
      std::cout << c.added << "\t"       //
                << c.removed << "\t"     //
                << c.added_test << "\t"  //
                << c.removed_test << "\t";
    }
    std::cout << commit.author << "\t"  //
              << commit.title << std::endl;
  });

  return EXIT_SUCCESS;
}
