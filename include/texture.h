#pragma once
#include "sdl_gl.h"
namespace mpv_glsl {
class Texture {
 public:
  Texture(int width, int height, int internalformat);
  ~Texture();
  Texture(const Texture&);
  Texture& operator=(const Texture&);

  unsigned int id;
};
}  // namespace mpv_glsl
