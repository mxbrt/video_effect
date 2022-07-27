#pragma once
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <sys/types.h>

#include <cstdint>

#include "api.h"
#include "sdl_gl.h"

namespace mpv_glsl {

struct window_ctx {
    SDL_Window *window;
    SDL_GLContext gl;
};

struct player_data {
   uint64_t draw_target_tick;  // SDL tick at which next frame should be drawn
   int playback_duration;      // Shorter videos will be looped to run at least
                               // playback_duration seconds
   bool playlist_next;         // true after video is changed
};

class Player {
   public:
    Player(struct window_ctx *ctx, const std::string &video_path,
           Api &api);
    ~Player();

    void play_file(const std::string &file_name);
    void run(struct window_ctx *ctx, SDL_Event event, unsigned int fbo,
             uint64_t &draw_target_tick, int playback_duration,
             bool &playlist_next);

   private:
    void load_playlist();
    Api &api;
    bool file_loaded;
    const std::string video_path;
    mpv_handle *mpv;
    mpv_render_context *mpv_gl;
    };
}  // namespace mpv_glsl
