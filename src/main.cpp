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
#include "config.h"
#include "fbo.h"
#include "gui.h"
#include "imgui_impl_sdl.h"
#include "player.h"
#include "shader.h"
#include "texture.h"
#include "util.h"
#include "vbo.h"

const int N_FINGERS = 10;

using namespace sendprotest;
using namespace std;

struct options {
    int shader_reload;
    int enable_gui;
    string media_path;
    string website_path;
    string config_path;
};

static struct options opts = {.shader_reload = 0, .enable_gui = 0};

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
        } else if (arg_str == "--config") {
            opt_idx++;
            if (opt_idx >= argc) {
                die("Missing value for option %s\n", arg_str.c_str());
            }
            opts.config_path = string(argv[opt_idx]);
        } else if (arg_str == "--shader-reload") {
            opts.shader_reload = 1;
        } else if (arg_str == "--enable-gui") {
            opts.enable_gui = 1;
        } else {
            die("Unknown argument: %s\n", argv[opt_idx]);
        }
    }

    if (opts.media_path.empty() && opts.website_path.empty() &&
        opts.config_path.empty()) {
        die("Must specify media, website & config paths");
    }

    if (!opts.media_path.ends_with("/")) {
        opts.media_path = opts.media_path + "/";
    }
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    struct window_ctx window_ctx;
    // gui values
    auto config = Config(opts.config_path);
    auto current_effect = config.get_selected_name();
    auto current_category = config.get_player_config().category;

    // initialize random seed
    srand (time(NULL));

    // Start API server
    auto api = Api(opts.media_path, opts.website_path, current_category);
    auto player = Player(&window_ctx, opts.media_path, api, current_category,
                         config.get_player_config().playback_duration);

    auto gui = Gui(window_ctx);

    string shader_macro = "#version 300 es\n#define " + current_effect + "\n";
    auto effect_shader =
        Shader("shaders/vert.glsl", "shaders/frag.glsl", shader_macro);
    auto input_shader =
        Shader("shaders/vert.glsl", "shaders/input.glsl", shader_macro);

    auto empty_vec = vector<float>();
    auto vbo = Vbo(empty_vec);
    // framebuffer configuration
    Fbo mpv_fbo = Fbo(width, height);
    DoubleFbo render_fbos = DoubleFbo(width, height);
    DoubleFbo effect_fbos = DoubleFbo(width, height, GL_R32F);

    // Precompute textures
    // Simplex Noise
    vector<Texture> simplex_textures;
    size_t simplex_idx = 0;
    {
        for (int i = 0; i < 16; i++) {
            auto macro = shader_macro + "\n#define OFFSET " +
                               to_string(i * 10000) + "\n";
            auto simplex_shader =
                Shader("shaders/vert.glsl", "shaders/simplex.glsl", macro);
            auto texture = Texture(width, height, GL_R32F);
            texture.render(simplex_shader);
            simplex_textures.push_back(texture);
        }
    }
    // Voronoi noise
    auto voronoise_texture = Texture(width * 4, height * 4);
    {
        auto voronoise_shader =
            Shader("shaders/vert.glsl", "shaders/voronoi.glsl", shader_macro);
        voronoise_texture.render(voronoise_shader);
    }

    uint64_t last_player_render = 1;
    uint64_t last_render_tick = SDL_GetTicks64();

    bool next_file = false;
    // Touch event state
    while (1) {
        SDL_Event event;
        bool imgui_event = false;
        while (SDL_PollEvent(&event)) {
            if (opts.enable_gui && gui.process_event(event)) {
                imgui_event = true;
            }
            switch (event.type) {
                case SDL_QUIT:
                    goto done;
                case SDL_WINDOWEVENT:
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_c) {
                        opts.enable_gui = !opts.enable_gui;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    break;
                default:
                    player.run(&window_ctx, event, mpv_fbo.fbo,
                               last_player_render,
                               config.get_player_config().playback_duration,
                               next_file);
                    break;
            }
        }

        auto api_play_file = api.get_play_cmd();
        if (api_play_file) {
            player.play_file(*api_play_file);
        }

        if (current_category != config.get_player_config().category) {
            current_category = config.get_player_config().category;
            api.set_category(current_category);
            player.set_category(current_category);
        }

        uint64_t ticks = SDL_GetTicks64();
        if ((ticks - last_player_render) / 1000 >
            (uint64_t)config.get_player_config().playback_duration + 5) {
            // Sometimes, mpv gets stuck after starting the first video (only on
            // rockpro64). Bail out and restart.
            die("mpv stuck\n");
        }

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

        if (current_effect != config.get_selected_name()) {
            current_effect = config.get_selected_name();
            stringstream ss;
            ss << "#version 300 es\n#define " << current_effect << "\n";
            effect_shader.set_macros(ss.str());
            input_shader.set_macros(ss.str());
        }

        if (next_file && current_category == COMBINED_CATEGORY) {
            config.set_random_effect();
            current_effect = config.get_selected_name();
            stringstream ss;
            ss << "#version 300 es\n#define " << current_effect << "\n";
            effect_shader.set_macros(ss.str());
            input_shader.set_macros(ss.str());
        }

        if (opts.shader_reload) {
            effect_shader.reload();
            input_shader.reload();
        }

        auto effect_data = config.get_selected_effect();

        uint64_t delta = ticks - last_render_tick;
        last_render_tick = ticks;
        effect_fbos.swap();
        render_fbos.swap();

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
                    effect_data.finger_radius);
        glUniform1f(glGetUniformLocation(input_shader.program, "effectFadeIn"),
                    effect_data.effect_fade_in);
        glUniform1i(glGetUniformLocation(input_shader.program, "reset"),
                    (int)next_file);

        glUniform1f(glGetUniformLocation(input_shader.program, "delta"), delta);
        glBindVertexArray(vbo.vao);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUseProgram(effect_shader.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mpv_fbo.texture.id);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, effect_fbos.get_front().texture.id);
        glActiveTexture(GL_TEXTURE2);
        auto simplex_texture_id = simplex_textures[simplex_idx].id;
        if (next_file) {
            simplex_idx = (simplex_idx + 1) % simplex_textures.size();
        }
        glBindTexture(GL_TEXTURE_2D, simplex_texture_id);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, render_fbos.get_back().texture.id);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, voronoise_texture.id);
        glBindFramebuffer(GL_FRAMEBUFFER, render_fbos.get_front().fbo);
        glDisable(GL_DEPTH_TEST);
        glUniform1i(
                glGetUniformLocation(effect_shader.program, "mpvTexture"), 0);
        glUniform1i(
            glGetUniformLocation(effect_shader.program, "effectTexture"), 1);
        glUniform1i(
            glGetUniformLocation(effect_shader.program, "simplexNoiseTexture"),
            2);
        glUniform1i(glGetUniformLocation(effect_shader.program, "lastFrame"),
                    3);
        glUniform1i(glGetUniformLocation(effect_shader.program, "voronoiNoise"),
                    4);
        glUniform2f(glGetUniformLocation(effect_shader.program, "resolution"),
                    width, height);
        glUniform1f(glGetUniformLocation(effect_shader.program, "amount"),
                    effect_data.effect_amount);
        glUniform1f(glGetUniformLocation(effect_shader.program, "time"),
                    ticks / 1000.0);

        glBindVertexArray(vbo.vao);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (opts.enable_gui) {
            gui.render(config);
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, render_fbos.get_front().fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        SDL_GL_SwapWindow(window_ctx.window);
        next_file = false;
        fflush(stdout);
        fflush(stderr);
    }
done:
    return 0;
}
