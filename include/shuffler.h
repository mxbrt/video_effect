#pragma once

#include <random>
#include <string>
#include <vector>

class Shuffler {
 public:
  Shuffler(const std::string &directory_path);

  std::string get();

 private:
  void shuffle();

  size_t cur_idx;
  std::string directory_path;
  std::vector<std::string> files;
  std::default_random_engine rng;
};
