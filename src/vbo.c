#include "vbo.h"

#include "sdl_gl.h"
#include "util.h"

void vbo_create(struct vbo* v, float* vertices, size_t vertices_size) {
  glGenVertexArrays(1, &v->vao);
  glGenBuffers(1, &v->vbo);
  glBindVertexArray(v->vao);
  glBindBuffer(GL_ARRAY_BUFFER, v->vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void*)(2 * sizeof(float)));
}

void vbo_free(struct vbo* v) {
  glDeleteVertexArrays(1, &v->vao);
  glDeleteBuffers(1, &v->vbo);
}
