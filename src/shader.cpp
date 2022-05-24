#include "shader.h"

#include <sys/stat.h>

#include <fstream>

#include "util.h"

Shader::Shader(const std::string &vert_shader_path,
               const std::string &frag_shader_path)
    : frag_path(frag_shader_path), vert_path(vert_shader_path) {
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

int Shader::compile(const std::string &path, int type) {
    std::ifstream ifs(path);
    if (ifs.fail()) {
        die("Could not open file %s %s\n", path.c_str(), strerror(errno));
    }
    std::string shader_src((std::istreambuf_iterator<char>(ifs)),
                           (std::istreambuf_iterator<char>()));

    unsigned int shader = glCreateShader(type);
    int success;
    char info[512];
    const char *shader_arg[2] = {shader_src.c_str(), NULL};
    glShaderSource(shader, 1, shader_arg, NULL);
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

void shader_create(struct shader *s, const char *vert_shader_path,
                   const char *frag_shader_path) {
}

void Shader::reload() {
    long mtime = max_mtime();
    if (mtime > src_mtime) {
        fprintf(stdout, "Reloading shaders\n");
        init();
    }
}
