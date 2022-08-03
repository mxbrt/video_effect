#pragma once
#include <stddef.h>

#include <vector>

namespace sendprotest {
using namespace std;
class Vbo {
 public:
  Vbo(std::vector<float>& vertices);
  ~Vbo();

  unsigned int vao;
  unsigned int vbo;
};
}  // namespace sendprotest
