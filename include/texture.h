#pragma once
#include "sdl_gl.h"
#include "shader.h"
namespace sendprotest {
class Texture {
 public:
  Texture(int width, int height, int internalformat = GL_RGB);
  void release();
  void render(Shader& shader);
  unsigned int id, width, height;
};
}  // namespace sendprotest
