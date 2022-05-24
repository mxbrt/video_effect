#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "fbo.h"
#include "player.h"
#include "shader.h"
#include "util.h"
#include "vbo.h"

float quadVertices[] = {  // vertex attributes for a quad that fills the entire
                          // screen in Normalized Device Coordinates.
    // positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

struct options {
    int shader_reload;
    char *media_path;
};

static struct options opts = {.shader_reload = 0};

void parse_args(int argc, char *argv[]) {
    if (argc < 2) die("pass a single media file as argument");
    for (int opt_idx = 1; opt_idx < argc - 1; opt_idx++) {
        if (!strcmp(argv[opt_idx], "--shader-reload")) {
            opts.shader_reload = 1;
        } else {
            die("Unknown argument: %s\n", argv[opt_idx]);
        }
    }
    opts.media_path = argv[argc - 1];
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    player_create();

    struct shader pixelization_shader;
    shader_create(&pixelization_shader, "shaders/vert.glsl",
                  "shaders/frag.glsl");


    glUniform1i(glGetUniformLocation(pixelization_shader.frag, "movieTexture"),
                0);

    struct vbo quad_vbo;
    vbo_create(&quad_vbo, quadVertices, sizeof(quadVertices));

    // framebuffer configuration
    struct fbo mpv_fbo;
    fbo_create(&mpv_fbo, 1920, 1080);

    // Play this file.
    const char *cmd[] = {"loadfile", opts.media_path, NULL};
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
                redraw = player_draw(event, mpv_fbo.fbo);
                break;
        }
        if (redraw) {
            if (opts.shader_reload) {
                shader_reload(&pixelization_shader);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(pixelization_shader.program);
            glad_glBindVertexArray(quad_vbo.vao);
            glBindTexture(GL_TEXTURE_2D, mpv_fbo.texture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            player_swap_window();
            }
        }
done:

    player_free();
    shader_free(&pixelization_shader);
    vbo_free(&quad_vbo);
    fbo_free(&mpv_fbo);
    return 0;
}
