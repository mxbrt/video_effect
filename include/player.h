#pragma once
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <sys/types.h>

#include <cstdint>

#include "api.h"
#include "sdl_gl.h"

namespace sendprotest {

enum class Command : uint64_t {
   Loadfile = 501,
   Shuffle = 502,
   Duration = 504,
   Filename = 505,
};

struct window_ctx {
   SDL_Window *window;
   SDL_GLContext gl;
};

class Player {
   public:
    Player(struct window_ctx *ctx, const std::string &video_path, Api &api,
           int category, int playback_duration);
    ~Player();

    void play_file(const std::string &file_name);
    void run(struct window_ctx *ctx, SDL_Event event, unsigned int fbo,
             uint64_t &draw_target_tick, int playback_duration,
             bool &playlist_next);
    void set_category(int new_category);

   private:
    void load_playlist();
    Api &api;
    int category;
    bool file_loaded;
    const std::string media_path;
    mpv_handle *mpv;
    mpv_render_context *mpv_gl;
    Command load_state;
    };
}  // namespace sendprotest
