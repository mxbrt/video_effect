#pragma once
#include <memory>

#include "texture.h"

namespace sendprotest {
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

class DoubleFbo {
 public:
  DoubleFbo(int width, int height, int internalformat = GL_RGB);

  Fbo& get_front();
  Fbo& get_back();
  void swap();

 private:
  Fbo fbos[2];
  size_t fbo_idx;
};

}  // namespace sendprotest
