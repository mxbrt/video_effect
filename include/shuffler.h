#pragma once

#include <random>
#include <string>
#include <vector>

namespace sendprotest {
using namespace std;
class Shuffler {
 public:
  Shuffler(const vector<string>& directories);
  vector<string> get();

 private:
  void read_directories();
  void shuffle();

  size_t cur_idx;
  vector<string> directories;
  vector<string> files;
  default_random_engine rng;
};
}  // namespace sendprotest
