#ifndef FBO_H
#define FBO_H

struct fbo {
  unsigned int fbo;
  unsigned int texture;
  unsigned int rbo;
};

void fbo_create(struct fbo *f, int width, int height);
void fbo_free(struct fbo *f);
#endif  // FBO_H
