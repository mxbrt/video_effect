#ifndef SHADER_H
#define SHADER_H
#include <string>

#include "sdl_gl.h"

class Shader {
 public:
  Shader(const std::string& vert_shader_path,
                 const std::string& frag_shader_path);
  ~Shader();
  void reload();

  int program;
 private:
  void init();
  void deinit();
  int compile(const std::string& path, int type);
  long max_mtime();

  int frag;
  int vert;
  std::string frag_path;
  std::string vert_path;
  int src_mtime;
  };

void shader_reload(struct shader* s);
#endif  // SHADER_H
