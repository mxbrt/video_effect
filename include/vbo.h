#pragma once
#include <stddef.h>

#include <vector>

namespace mpv_glsl {
using namespace std;
class Vbo {
 public:
  Vbo(std::vector<float>& vertices);
  ~Vbo();

  unsigned int vao;
  unsigned int vbo;
};
}  // namespace mpv_glsl
