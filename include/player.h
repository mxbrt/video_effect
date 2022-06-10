#pragma once
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include "sdl_gl.h"

namespace mpv_glsl {

struct window_ctx {
    SDL_Window *window;
    SDL_GLContext gl;
};

enum player_event {
    PLAYER_REDRAW,
    PLAYER_IDLE,
    PLAYER_NO_EVENT,
};

class Player {
   public:
    Player(struct window_ctx *ctx);
    ~Player();

    void cmd(const char **cmd);
    enum player_event run(struct window_ctx *ctx, SDL_Event event,
                          unsigned int fbo);

   private:
    mpv_handle *mpv;
    mpv_render_context *mpv_gl;
};
}  // namespace mpv_glsl
