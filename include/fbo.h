#pragma once
#include <memory>

#include "texture.h"

namespace mpv_glsl {
using namespace std;
class Fbo {
 public:
  Fbo(int width, int height, int internalformat = GL_RGB);
  ~Fbo();
  Fbo(const Fbo&) = delete;
  Fbo& operator=(const Fbo&) = delete;

  Texture texture;
  unsigned int fbo;

 private:
  unsigned int rbo;
};
}  // namespace mpv_glsl
