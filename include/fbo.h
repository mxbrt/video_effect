#pragma once
#include "texture.h"

namespace mpv_glsl {
class Fbo {
 public:
  Fbo(int width, int height);
  ~Fbo();

  Texture texture;
  unsigned int fbo;

 private:
  unsigned int rbo;
};
}  // namespace mpv_glsl
