#include <errno.h>
#include <mpv/client.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <cctype>
#include <cstdint>
#include <cstring>
#include <sstream>
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
    auto shuffler =
        Shuffler({opts.media_path + "video", opts.media_path + "image"});

    // Start API server
    auto api = Api(opts.media_path, opts.website_path);
    auto player = Player(&window_ctx, opts.media_path + "video", api);
    auto gui = Gui(window_ctx);

    auto effect_macro = "#version 300 es\n#define Swirl\n";
    auto effect_shader =
        Shader("shaders/vert.glsl", "shaders/frag.glsl", effect_macro);
    auto input_shader = Shader("shaders/vert.glsl", "shaders/input.glsl");

    auto empty_vec = vector<float>();
    auto vbo = Vbo(empty_vec);
    // framebuffer configuration
    DoubleFbo mpv_fbos = DoubleFbo(width, height);
    DoubleFbo effect_fbos = DoubleFbo(width, height, GL_R32F);

    // Noise textures
    // FIXME: cleanup render pass resources
    auto simplex_noise_texture = Texture(width, height, GL_R32F);
    {
        auto noise_shader =
            Shader("shaders/vert.glsl", "shaders/noise.glsl", effect_macro);
        simplex_noise_texture.render(noise_shader);
    }

    // gui values
    vector<EffectItem> effects = {
        {.name = "Swirl", .is_selected = true},
        {.name = "Blur", .is_selected = false},
        {.name = "Pixel", .is_selected = false},
        {.name = "Voronoi", .is_selected = false},
        {.name = "Brushed", .is_selected = false},
        {.name = "Debug", .is_selected = false},
    };
    size_t default_effect = 0;
    auto gui_data = GuiData{
        .finger_radius = 0.20,
        .effect_fade_in = 0.2,
        .effect_amount = 10.0,
        .effects = effects,
        .selected_effect = default_effect,
        .playback_duration = 20,
    };
    size_t loaded_effect = default_effect;

    uint64_t player_target_tick = 1;
    uint64_t last_player_swap = 0;
    uint64_t last_render_tick = SDL_GetTicks64();

    bool reset_effect = false;
    // Touch event state
    while (1) {
        SDL_Event event;
        bool imgui_event = false;
        while (SDL_PollEvent(&event)) {
            if (gui.process_event(event)) {
                imgui_event = true;
            }
            switch (event.type) {
                case SDL_QUIT:
                    goto done;
                case SDL_WINDOWEVENT:
                    break;
                case SDL_KEYDOWN:
                    break;
                case SDL_MOUSEMOTION:
                    break;
                default:
                    player.run(&window_ctx, event, mpv_fbos.get_back().fbo,
                               player_target_tick, gui_data.playback_duration,
                               reset_effect);
                    if (player_target_tick > last_player_swap) {
                        player_target_tick = last_player_swap;
                        mpv_fbos.swap();
                    }
                    break;
            }
        }

        auto api_play_file = api.get_play_cmd();
        if (api_play_file) {
            player.play_file(*api_play_file);
        }

        uint64_t ticks = SDL_GetTicks64();
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

        if (loaded_effect != gui_data.selected_effect) {
            auto effect = gui_data.effects[gui_data.selected_effect];
            stringstream ss;
            ss << "#version 300 es\n#define " << effect.name << "\n";
            effect_shader.set_macros(ss.str());
            loaded_effect = gui_data.selected_effect;
        }

        if (opts.shader_reload) {
            effect_shader.reload();
            input_shader.reload();
        }

        uint64_t delta = ticks - last_render_tick;
        last_render_tick = ticks;
        effect_fbos.swap();

        glUseProgram(input_shader.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, effect_fbos.get_back().texture.id);
        glBindFramebuffer(GL_FRAMEBUFFER, effect_fbos.get_front().fbo);
        glDisable(GL_DEPTH_TEST);

        glUniform1i(glGetUniformLocation(input_shader.program, "effectTexture"),
                    0);
        glUniform2f(glGetUniformLocation(input_shader.program, "resolution"),
                    width, height);
        glUniform3fv(glGetUniformLocation(input_shader.program, "fingers"),
                     N_FINGERS, &fingers_uniform[0][0]);
        glUniform1f(glGetUniformLocation(input_shader.program, "fingerRadius"),
                    gui_data.finger_radius);
        glUniform1f(glGetUniformLocation(input_shader.program, "effectFadeIn"),
                    gui_data.effect_fade_in);
        glUniform1i(glGetUniformLocation(input_shader.program, "reset"),
                    (int)reset_effect);
        reset_effect = false;
        glUniform1f(glGetUniformLocation(input_shader.program, "delta"), delta);
        glBindVertexArray(vbo.vao);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUseProgram(effect_shader.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mpv_fbos.get_front().texture.id);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, effect_fbos.get_front().texture.id);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, simplex_noise_texture.id);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glUniform1i(glGetUniformLocation(effect_shader.program, "movieTexture"),
                    0);
        glUniform1i(
            glGetUniformLocation(effect_shader.program, "effectTexture"), 1);
        glUniform1i(
            glGetUniformLocation(effect_shader.program, "simplexNoiseTexture"),
            2);
        glUniform2f(glGetUniformLocation(effect_shader.program, "resolution"),
                    width, height);
        glUniform1f(glGetUniformLocation(effect_shader.program, "amount"),
                    gui_data.effect_amount);
        glUniform1f(glGetUniformLocation(effect_shader.program, "time"),
                    ticks / 1000.0);

        glBindVertexArray(vbo.vao);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        gui.render(gui_data);
        SDL_GL_SwapWindow(window_ctx.window);
    }
done:
    return 0;
}
