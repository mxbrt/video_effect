#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstdint>
#include <cstring>
#include <string>

#include "SDL_events.h"
#include "SDL_touch.h"
#include "api.h"
#include "fbo.h"
#include "gui.h"
#include "imgui_impl_sdl.h"
#include "player.h"
#include "shader.h"
#include "shuffler.h"
#include "texture.h"
#include "util.h"
#include "vbo.h"

const int N_FINGERS = 10;

using namespace mpv_glsl;
using namespace std;

vector<float> quadVertices = {  // vertex attributes for a quad that fills
                                     // the entire screen in Normalized Device
                                     // Coordinates. positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

struct options {
    int shader_reload;
    string media_path;
    string website_path;
};

static struct options opts = {.shader_reload = 0};

static const int width = 1920;
static const int height = 1080;

void parse_args(int argc, char *argv[]) {
    for (int opt_idx = 1; opt_idx < argc; opt_idx++) {
        auto arg_str = string(argv[opt_idx]);
        if (arg_str == "--media-dir") {
            opt_idx++;
            if (opt_idx >= argc) {
                die("Missing value for option %s\n", arg_str.c_str());
            }
            opts.media_path = string(argv[opt_idx]);
        } else if (arg_str == "--website-dir") {
            opt_idx++;
            if (opt_idx >= argc) {
                die("Missing value for option %s\n", arg_str.c_str());
            }
            opts.website_path = string(argv[opt_idx]);
        } else if (arg_str == "--shader-reload") {
            opts.shader_reload = 1;
        } else {
            die("Unknown argument: %s\n", argv[opt_idx]);
        }
    }

    if (opts.media_path.empty() && opts.website_path.empty()) {
        die("Must specify media & website directory");
    }
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    struct window_ctx window_ctx;
    player_create(&window_ctx);
    auto gui = Gui(window_ctx);

    auto pixelization_shader = Shader("shaders/vert.glsl", "shaders/frag.glsl");
    auto input_shader = Shader("shaders/vert.glsl", "shaders/input.glsl");

    auto quad_vbo = Vbo(quadVertices);
    // framebuffer configuration
    auto mpv_fbo = Fbo(width, height);
    Fbo effect_fbo[2] = {Fbo(width, height), Fbo(width, height)};
    int effect_fbo_idx = 0;

    // gui values
    auto gui_data = GuiData{
        .finger_radius = 0.15,
        .effect_fade_in = 0.1,
        .effect_fade_out = 0.1,
        .pixelization = 28.0,
        .input_debug = false,
    };

    // Play this file.
    auto shuffler =
        Shuffler({opts.media_path + "video", opts.media_path + "image"});
    auto video_path = shuffler.get();
    const char *cmd[] = {"loadfile", video_path.c_str(), NULL};
    player_cmd(cmd);

    uint64_t last_render = SDL_GetTicks64();
    uint64_t target_frametime = 1000 / 60;

    // Start API server
    auto api = Api(opts.media_path, opts.website_path);

    // Touch event state
    while (1) {
        SDL_Event event;
        enum player_event player_event = PLAYER_NO_EVENT;
        SDL_WaitEventTimeout(&event, target_frametime);
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

        bool imgui_event = gui.process_event(event);

        auto play_command = api.get_play_cmd();
        if (play_command) {
            const char *cmd[] = {"loadfile", play_command->c_str(), NULL};
            player_cmd(cmd);
        } else if (player_event == PLAYER_IDLE) {
            auto media_path = shuffler.get();
            const char *cmd[] = {"loadfile", media_path.c_str(), NULL};
            player_cmd(cmd);
            api.set_play_cmd(media_path.c_str());
        }

        uint64_t ticks = SDL_GetTicks64();
        uint64_t render_delta = (ticks - last_render);

        if (player_event == PLAYER_REDRAW || render_delta >= target_frametime) {
            last_render = ticks;
            uint32_t buttons;
            SDL_PumpEvents();  // make sure we have the latest input state.
            float fingers_uniform[N_FINGERS][3] = {};
            for (int i = 0; i < N_FINGERS; i++) {
                // We initialize the z value to a high value. It will be set to
                // zero, if a click or touch is active. Consequently, finger
                // positions that are not active will have a very high distance
                // to the xy plane.
                fingers_uniform[i][2] = 1000000;
            }
            if (!imgui_event) {
                int mouse_x = 0, mouse_y = 0;
                buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
                if ((buttons & SDL_BUTTON_LMASK) != 0) {
                    fingers_uniform[0][0] = mouse_x;
                    fingers_uniform[0][1] = height - mouse_y;
                    fingers_uniform[0][2] = 0.0;
                }

                if (SDL_GetNumTouchDevices() > 0) {
                    int touch_id = SDL_GetTouchDevice(0);
                    if (touch_id == 0) {
                        die("Invalid touch device\n");
                    }
                    int n_touch_fingers = SDL_GetNumTouchFingers(touch_id);
                    for (int i = 0; i < n_touch_fingers; i++) {
                        auto finger = SDL_GetTouchFinger(touch_id, i);
                        if (finger == NULL) {
                            die("Failed to get finger\n");
                        }
                        fingers_uniform[i][0] = finger->x * width;
                        fingers_uniform[i][1] = height - finger->y * height;
                        fingers_uniform[i][2] = 0.0;
                    }
                }
            }

            if (opts.shader_reload) {
                pixelization_shader.reload();
                input_shader.reload();
            }

            auto &last_effect_fbo = effect_fbo[effect_fbo_idx];
            auto &cur_effect_fbo = effect_fbo[(effect_fbo_idx + 1) % 2];
            effect_fbo_idx = (effect_fbo_idx + 1) % 2;

            glUseProgram(input_shader.program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, last_effect_fbo.texture.id);
            glBindFramebuffer(GL_FRAMEBUFFER, cur_effect_fbo.fbo);
            glDisable(GL_DEPTH_TEST);

            glUniform1i(
                glGetUniformLocation(input_shader.program, "effectTexture"), 0);
            glUniform2f(
                glGetUniformLocation(input_shader.program, "resolution"), width,
                height);
            glUniform3fv(glGetUniformLocation(input_shader.program, "fingers"),
                         N_FINGERS, &fingers_uniform[0][0]);
            glUniform1f(
                glGetUniformLocation(input_shader.program, "fingerRadius"),
                gui_data.finger_radius);
            glUniform1f(
                glGetUniformLocation(input_shader.program, "effectFadeIn"),
                gui_data.effect_fade_in);
            glUniform1f(
                glGetUniformLocation(input_shader.program, "effectFadeOut"),
                gui_data.effect_fade_out);
            glad_glBindVertexArray(quad_vbo.vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glUseProgram(pixelization_shader.program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mpv_fbo.texture.id);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, cur_effect_fbo.texture.id);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUniform1i(glGetUniformLocation(pixelization_shader.program,
                                             "movieTexture"),
                        0);
            glUniform1i(glGetUniformLocation(pixelization_shader.program,
                                             "effectTexture"),
                        1);
            glUniform2f(
                glGetUniformLocation(pixelization_shader.program, "resolution"),
                width, height);
            glUniform1f(glGetUniformLocation(pixelization_shader.program,
                                             "pixelization"),
                        gui_data.pixelization);
            glUniform1i(
                glGetUniformLocation(pixelization_shader.program, "inputDebug"),
                gui_data.input_debug);

            glad_glBindVertexArray(quad_vbo.vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            gui.render(gui_data);
            SDL_GL_SwapWindow(window_ctx.window);
        }
        }
done:

    player_free();
    return 0;
}
