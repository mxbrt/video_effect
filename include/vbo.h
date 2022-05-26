#pragma once
#include <stddef.h>

#include <vector>

class Vbo {
 public:
  Vbo(std::vector<float>& vertices);
  ~Vbo();

  unsigned int vao;
  unsigned int vbo;
};
