#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "player.h"

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

const int compile_shader(const char *path, int type) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        die("Could not open shader");
    }
    char shader_src[4096 * 1024] = {0};
    size_t src_len = fread(shader_src, 1, sizeof(shader_src), fp);
    if (src_len >= sizeof(shader_src)) {
        die("Shader too large");
    }
    const char *shader_arg[2] = {shader_src, NULL};

    unsigned int shader = glCreateShader(type);
    int success;
    char info[512];
    glShaderSource(shader, 1, shader_arg, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info);
        fprintf(stderr, "shader compilation failed\n%s\n", info);
        exit(1);
    }
    return shader;
}

const int init_shader_program(const int vert_shader, const int frag_shader) {
    int success;
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vert_shader);
    glAttachShader(shader_program, frag_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info);
        fprintf(stderr, "shader linking failed\n\n%s", info);
        exit(1);
    }
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    return shader_program;
}

float quadVertices[] = {  // vertex attributes for a quad that fills the entire
                          // screen in Normalized Device Coordinates.
    // positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

int main(int argc, char *argv[]) {
    if (argc != 2) die("pass a single media file as argument");

    player_create();

    // shader configuration
    const int vert_shader =
        compile_shader("shaders/vert.glsl", GL_VERTEX_SHADER);
    const int frag_shader =
        compile_shader("shaders/frag.glsl", GL_FRAGMENT_SHADER);
    const int shader_program = init_shader_program(vert_shader, frag_shader);

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));

    glUniform1i(glGetUniformLocation(frag_shader, "movieTexture"), 0);

    // framebuffer configuration
    unsigned int mpv_fbo;
    glGenFramebuffers(1, &mpv_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mpv_fbo);
    unsigned int mpv_texture;
    glGenTextures(1, &mpv_texture);
    glBindTexture(GL_TEXTURE_2D, mpv_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           mpv_texture, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1920, 1080);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        die("Framebuffer incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Play this file.
    const char *cmd[] = {"loadfile", argv[1], NULL};
    player_cmd(cmd);

    while (1) {
        SDL_Event event;
        if (SDL_WaitEvent(&event) != 1) die("event loop error");
        int redraw = 0;
        switch (event.type) {
            case SDL_QUIT:
                goto done;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED) redraw = 1;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_SPACE) {
                    const char *cmd_pause[] = {"cycle", "pause", NULL};
                    player_cmd(cmd_pause);
                }
                break;
            default:
                redraw = player_draw(event, mpv_fbo);
                break;
        }
        if (redraw) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(shader_program);
            glad_glBindVertexArray(quadVAO);
            glBindTexture(GL_TEXTURE_2D, mpv_texture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            player_swap_window();
        }
    }
done:

    player_free();
    glDeleteFramebuffers(1, &mpv_fbo);
    return 0;
}
