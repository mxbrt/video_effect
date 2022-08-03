#include "shader.h"

#include <sys/stat.h>

#include <fstream>

#include "util.h"

namespace sendprotest {
using namespace std;

Shader::Shader(const string &vert_shader_path, const string &frag_shader_path,
               const string &macros)
    : frag_path(frag_shader_path), vert_path(vert_shader_path), macros(macros) {
    init();
}

Shader::~Shader() { deinit(); }

void Shader::deinit() {
    glDeleteShader(vert);
    glDeleteShader(frag);
    glDeleteProgram(program);
}

void Shader::init() {
    int success;
    vert = compile(vert_path, GL_VERTEX_SHADER);
    frag = compile(frag_path, GL_FRAGMENT_SHADER);
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vert);
    glAttachShader(shader_program, frag);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info);
        die("shader linking failed\n\n%s", info);
    }
    program = shader_program;
    src_mtime = max_mtime();
}

int Shader::compile(const string &path, int type) {
    ifstream ifs(path);
    if (ifs.fail()) {
        die("Could not open file %s %s\n", path.c_str(), strerror(errno));
    }
    string shader_src((istreambuf_iterator<char>(ifs)),
                           (istreambuf_iterator<char>()));

    unsigned int shader = glCreateShader(type);
    int success;
    char info[512];
    const char *shader_arg[3] = {macros.c_str(), shader_src.c_str(), NULL};
    glShaderSource(shader, 2, shader_arg, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info);
        die("%s: compilation error\n%s\n", path.c_str(), info);
    }
    return shader;
}

long Shader::max_mtime() {
    long vert_mtime = get_mtime(vert_path.c_str());
    long frag_mtime = get_mtime(frag_path.c_str());
    long max_mtime = vert_mtime > frag_mtime ? vert_mtime : frag_mtime;
    return max_mtime;
}

void Shader::reload() {
    long mtime = max_mtime();
    if (mtime > src_mtime) {
        fprintf(stdout, "Reloading shaders\n");
        init();
    }
}

void Shader::set_macros(const string &new_macros) {
    macros = new_macros;
    init();
}
}  // namespace sendprotest
