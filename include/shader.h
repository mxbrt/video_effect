#pragma once
#include <string>
#include <vector>

#include "sdl_gl.h"

namespace sendprotest {
using namespace std;
class Shader {
 public:
  Shader(const string& vert_shader_path, const string& frag_shader_path,
         const string& macros = "#version 300 es\n");
  ~Shader();
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

  void reload();
  void set_macros(const string& define);

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
  string macros;
  };
}  // namespace sendprotest
