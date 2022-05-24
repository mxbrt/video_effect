#ifndef SHADER_H
#define SHADER_H
#include "sdl_gl.h"
struct shader {
  int frag;
  int vert;
  int program;
  char* frag_path;
  char* vert_path;
  int src_mtime;
};

void shader_create(struct shader* s, const char* vert_shader_path,
                   const char* frag_shader_path);
void shader_free(struct shader* s);

void shader_reload(struct shader* s);
#endif  // SHADER_H
