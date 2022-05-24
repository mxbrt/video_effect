#ifndef VBO_H
#define VBO_H

#include <stddef.h>

struct vbo {
  unsigned int vao;
  unsigned int vbo;
};

void vbo_create(struct vbo* v, float* vertices, size_t vertices_size);
void vbo_free(struct vbo *f);
#endif  // VBO_H
