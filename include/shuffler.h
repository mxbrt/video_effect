#pragma once

#include <random>
#include <string>
#include <vector>

class Shuffler {
 public:
  Shuffler(const std::vector<std::string>& directories);

  std::string get();

 private:
  void read_directories();
  void shuffle();

  size_t cur_idx;
  std::vector<std::string> directories;
  std::vector<std::string> files;
  std::default_random_engine rng;
};
