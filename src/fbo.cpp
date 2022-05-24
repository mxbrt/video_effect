#include "fbo.h"

#include "sdl_gl.h"
#include "texture.h"
#include "util.h"

Fbo::Fbo(int width, int height) : texture(Texture(width, height)) {
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture.id, 0);

  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    die("Framebuffer incomplete");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Fbo::~Fbo() {
  glDeleteRenderbuffers(1, &rbo);
  glDeleteFramebuffers(1, &fbo);
}
