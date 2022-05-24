#ifndef FBO_H
#define FBO_H

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
#endif  // FBO_H
