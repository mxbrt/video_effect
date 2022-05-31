#pragma once
#include <string>

#include "sdl_gl.h"

namespace mpv_glsl {
using namespace std;
class Shader {
 public:
  Shader(const string& vert_shader_path,
                 const string& frag_shader_path);
  ~Shader();
  void reload();

  int program;
 private:
  void init();
  void deinit();
  int compile(const string& path, int type);
  long max_mtime();

  int frag;
  int vert;
  string frag_path;
  string vert_path;
  int src_mtime;
  };

void shader_reload(struct shader* s);
}  // namespace mpv_glsl
