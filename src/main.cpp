#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "fbo.h"
#include "gui.h"
#include "player.h"
#include "shader.h"
#include "shuffler.h"
#include "texture.h"
#include "util.h"
#include "vbo.h"

std::vector<float> quadVertices = {  // vertex attributes for a quad that fills
                                     // the entire screen in Normalized Device
                                     // Coordinates. positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

struct options {
    int shader_reload;
    std::string video_path;
};

static struct options opts = {.shader_reload = 0};

static const int width = 1920;
static const int height = 1080;
static int frame = 0;

void parse_args(int argc, char *argv[]) {
    for (int opt_idx = 1; opt_idx < argc; opt_idx++) {
        auto arg_str = std::string(argv[opt_idx]);
        if (arg_str == "--video-dir") {
            opt_idx++;
            if (opt_idx >= argc) {
                die("Missing value for option %s\n", arg_str.c_str());
            }
            opts.video_path = std::string(argv[opt_idx]);
        } else if (arg_str == "--shader-reload") {
            opts.shader_reload = 1;
        } else {
            die("Unknown argument: %s\n", argv[opt_idx]);
        }
    }

    if (opts.video_path.empty()) {
        die("Must specify video directory");
    }
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    struct window_ctx window_ctx;
    player_create(&window_ctx);
    auto gui = Gui(window_ctx);

    auto pixelization_shader = Shader("shaders/vert.glsl", "shaders/frag.glsl");
    auto mouse_shader = Shader("shaders/vert.glsl", "shaders/mouse.glsl");

    auto quad_vbo = Vbo(quadVertices);
    // framebuffer configuration
    auto mpv_fbo = Fbo(width, height);
    Fbo mouse_fbo[2] = {Fbo(width, height), Fbo(width, height)};
    int mouse_fbo_idx = 0;

    // gui values
    float pixelization = 28.0;

    // Play this file.
    auto shuffler = Shuffler(opts.video_path);
    auto video_path = shuffler.get();
    const char *cmd[] = {"loadfile", video_path.c_str(), NULL};
    player_cmd(cmd);

    while (1) {
        SDL_Event event;
        enum player_event player_event = PLAYER_NO_EVENT;
        if (SDL_WaitEvent(&event) != 1) die("event loop error");
        gui.process_event(event);
        switch (event.type) {
            case SDL_QUIT:
                goto done;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
                    player_event = PLAYER_REDRAW;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_SPACE) {
                    const char *cmd_pause[] = {"cycle", "pause", NULL};
                    player_cmd(cmd_pause);
                }
                break;
            default:
                player_event = player_run(&window_ctx, event, mpv_fbo.fbo);
                break;
        }

        if (player_event == PLAYER_IDLE) {
            auto video_path = shuffler.get();
            const char *cmd[] = {"loadfile", video_path.c_str(), NULL};
            player_cmd(cmd);
        }

        if (player_event == PLAYER_REDRAW) {
            int x, y;
            uint32_t buttons;
            int mouse_click = 0;
            SDL_PumpEvents();  // make sure we have the latest mouse state.
            buttons = SDL_GetMouseState(&x, &y);
            if ((buttons & SDL_BUTTON_LMASK) != 0) {
                mouse_click = 1;
            }

            if (opts.shader_reload) {
                pixelization_shader.reload();
                mouse_shader.reload();
            }

            auto &last_mouse_fbo = mouse_fbo[mouse_fbo_idx];
            auto &cur_mouse_fbo = mouse_fbo[(mouse_fbo_idx + 1) % 2];
            mouse_fbo_idx = (mouse_fbo_idx + 1) % 2;

            glUseProgram(mouse_shader.program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, last_mouse_fbo.texture.id);
            glBindFramebuffer(GL_FRAMEBUFFER, cur_mouse_fbo.fbo);
            glDisable(GL_DEPTH_TEST);

            glUniform1i(
                glGetUniformLocation(mouse_shader.program, "mouseTexture"), 0);
            glUniform2f(
                glGetUniformLocation(mouse_shader.program, "resolution"), width,
                height);
            glUniform3f(glGetUniformLocation(mouse_shader.program, "mouse"), x,
                        height - y, mouse_click);
            glad_glBindVertexArray(quad_vbo.vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glUseProgram(pixelization_shader.program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mpv_fbo.texture.id);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, cur_mouse_fbo.texture.id);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUniform1i(glGetUniformLocation(pixelization_shader.program,
                                             "movieTexture"),
                        0);
            glUniform1i(glGetUniformLocation(pixelization_shader.program,
                                             "mouseTexture"),
                        1);
            glUniform2f(
                glGetUniformLocation(pixelization_shader.program, "resolution"),
                width, height);
            glUniform1f(
                glGetUniformLocation(pixelization_shader.program, "time"),
                (float)frame);
            glUniform1f(glGetUniformLocation(pixelization_shader.program,
                                             "pixelization"),
                        pixelization);

            glad_glBindVertexArray(quad_vbo.vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            gui.render(pixelization);
            SDL_GL_SwapWindow(window_ctx.window);
        }
    }
done:

    player_free();
    return 0;
}
