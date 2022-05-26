#pragma once
#include "texture.h"

class Fbo {
 public:
  Fbo(int width, int height);
  ~Fbo();

  Texture texture;
  unsigned int fbo;

 private:
  unsigned int rbo;
};
