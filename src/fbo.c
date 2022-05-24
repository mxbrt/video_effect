#include "fbo.h"

#include "sdl_gl.h"
#include "texture.h"
#include "util.h"

void fbo_create(struct fbo* f, int width, int height) {
  glGenFramebuffers(1, &f->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, f->fbo);

  f->texture = texture_create(width, height);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         f->texture, 0);

  glGenRenderbuffers(1, &f->rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, f->rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, f->rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    die("Framebuffer incomplete");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void fbo_free(struct fbo* f) {
  texture_free(f->texture);
  glDeleteRenderbuffers(1, &f->rbo);
  glDeleteFramebuffers(1, &f->fbo);
}
