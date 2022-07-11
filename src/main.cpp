#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstdint>
#include <cstring>
#include <string>

#include "SDL_events.h"
#include "SDL_timer.h"
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

    if (!opts.media_path.ends_with("/")) {
        opts.media_path = opts.media_path + "/";
    }
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    struct window_ctx window_ctx;
    auto player = Player(&window_ctx);
    auto gui = Gui(window_ctx);

    auto pixelization_shader = Shader("shaders/vert.glsl", "shaders/frag.glsl");
    auto input_shader = Shader("shaders/vert.glsl", "shaders/input.glsl");

    auto quad_vbo = Vbo(quadVertices);
    // framebuffer configuration
    Fbo mpv_fbo[2] = {Fbo(width, height), Fbo(width, height)};
    Fbo effect_fbo[2] = {Fbo(width, height), Fbo(width, height)};
    int effect_fbo_idx = 0;
    int mpv_fbo_idx = 0;

    // gui values
    auto gui_data = GuiData{
        .finger_radius = 0.15,
        .effect_fade_in = 0.1,
        .effect_fade_out = 0.1,
        .pixelization = 1.0,
        .input_debug = false,
    };

    // Play this file.
    auto shuffler =
        Shuffler({opts.media_path + "video", opts.media_path + "video"});
    auto video_path = shuffler.get();
    const char *cmd[] = {"loadfile", video_path.c_str(), NULL};
    player.cmd(cmd);

    uint64_t target_frametime = 1000 / 50;
    uint64_t render_target_tick = SDL_GetTicks64();
    uint64_t player_target_tick = 1;
    uint64_t last_player_swap = 0;

    // Start API server
    auto api = Api(opts.media_path, opts.website_path);

    // Touch event state
    while (1) {
        auto &cur_mpv_fbo = mpv_fbo[mpv_fbo_idx];
        auto &next_mpv_fbo = mpv_fbo[(mpv_fbo_idx + 1) % 2];
        bool window_exposed = false;

        enum player_event player_event = PLAYER_NO_EVENT;
        SDL_Event event;
        bool imgui_event = false;
        while (SDL_PollEvent(&event)) {
            imgui_event = imgui_event || gui.process_event(event);
            switch (event.type) {
                case SDL_QUIT:
                    goto done;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                        window_exposed = true;
                    }
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        const char *cmd_pause[] = {"cycle", "pause", NULL};
                        player.cmd(cmd_pause);
                    }
                    break;
                default:
                    player_event =
                        player.run(&window_ctx, event, next_mpv_fbo.fbo,
                                   player_target_tick);
                    if (player_event == PLAYER_IDLE) {
                        auto media_path = shuffler.get();
                        const char *cmd[] = {"loadfile", media_path.c_str(),
                                             NULL};
                        player.cmd(cmd);
                        api.set_play_cmd(media_path.c_str());
                        player_event = PLAYER_NO_EVENT;
                    }
                    break;
            }
        }

        auto play_command = api.get_play_cmd();
        if (play_command) {
            const char *cmd[] = {"loadfile", play_command->c_str(), NULL};
            player.cmd(cmd);
        }

        uint64_t ticks = SDL_GetTicks64();
        if (ticks >= player_target_tick &&
            player_target_tick > last_player_swap) {
            last_player_swap = ticks;
            mpv_fbo_idx = (mpv_fbo_idx + 1) % 2;
            cur_mpv_fbo = mpv_fbo[mpv_fbo_idx];
            next_mpv_fbo = mpv_fbo[(mpv_fbo_idx + 1) % 2];
        }

        if (ticks > render_target_tick || window_exposed) {
            render_target_tick = ticks + target_frametime - 1;
            uint32_t buttons;
            SDL_PumpEvents();  // make sure we have the latest input state.
            float fingers_uniform[N_FINGERS][3] = {};
            for (int i = 0; i < N_FINGERS; i++) {
                // We initialize the z value to a high value. It will be set
                // to zero, if a click or touch is active. Consequently,
                // finger positions that are not active will have a very
                // high distance to the xy plane.
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
            glBindTexture(GL_TEXTURE_2D, cur_mpv_fbo.texture.id);
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
    return 0;
}
