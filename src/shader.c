#include "shader.h"

#include <sys/stat.h>

#include "util.h"

static int shader_compile(const char *path, int type) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        die("Could not open shader");
    }

    char shader_src[1024 * 1024] = {0};
    size_t src_len = fread(shader_src, 1, sizeof(shader_src), fp);
    if (src_len >= sizeof(shader_src)) {
        die("Shader too large");
    }
    const char *shader_arg[2] = {shader_src, NULL};

    fclose(fp);

    unsigned int shader = glCreateShader(type);
    int success;
    char info[512];
    glShaderSource(shader, 1, shader_arg, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info);
        die("%s: compilation error\n%s\n", path, info);
    }
    return shader;
}

static long shader_max_mtime(const char *vert_shader_path,
                             const char *frag_shader_path) {
    long vert_mtime = get_mtime(vert_shader_path);
    long frag_mtime = get_mtime(frag_shader_path);
    long max_mtime = vert_mtime > frag_mtime ? vert_mtime : frag_mtime;
    return max_mtime;
}

void shader_create(struct shader *s, const char *vert_shader_path,
                   const char *frag_shader_path) {
    int success;
    s->vert = shader_compile(vert_shader_path, GL_VERTEX_SHADER);
    s->frag = shader_compile(frag_shader_path, GL_FRAGMENT_SHADER);
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, s->vert);
    glAttachShader(shader_program, s->frag);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info);
        die("shader linking failed\n\n%s", info);
    }
    s->program = shader_program;
    s->src_mtime = shader_max_mtime(vert_shader_path, frag_shader_path);
    s->vert_path = strdup(vert_shader_path);
    s->frag_path = strdup(frag_shader_path);
}

void shader_reload(struct shader *s) {
    long mtime = shader_max_mtime(s->vert_path, s->frag_path);
    if (mtime > s->src_mtime) {
        fprintf(stdout, "Reloading shaders\n");
        struct shader new_s;
        shader_create(&new_s, s->vert_path, s->frag_path);
        shader_free(s);
        memcpy(s, &new_s, sizeof(struct shader));
    }
}

void shader_free(struct shader *s) {
    glDeleteShader(s->vert);
    glDeleteShader(s->frag);
    glDeleteProgram(s->program);
    free(s->frag_path);
    free(s->vert_path);
}
