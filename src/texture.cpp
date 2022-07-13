#include "texture.h"

#include "fbo.h"
#include "util.h"

namespace mpv_glsl {
Texture::Texture(int width, int height, int internalformat)
    {
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::~Texture() {
  glDeleteTextures(1, &id);
}
}  // namespace mpv_glsl
