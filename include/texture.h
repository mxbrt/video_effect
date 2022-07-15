#pragma once
#include "sdl_gl.h"
#include "shader.h"
namespace mpv_glsl {
class Texture {
 public:
  Texture(int width, int height, int internalformat = GL_RGB);
  ~Texture();
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  void render(Shader& shader);

  unsigned int id, width, height;
};
}  // namespace mpv_glsl
