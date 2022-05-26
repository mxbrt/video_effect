#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sdl_gl.h"

struct window_ctx {
  SDL_Window *window;
  SDL_GLContext gl;
};

enum player_event {
  PLAYER_REDRAW,
  PLAYER_IDLE,
  PLAYER_NO_EVENT,
};

void player_create(struct window_ctx *w);
void player_free();

void player_cmd(const char *cmd[]);
enum player_event player_run(struct window_ctx *w, SDL_Event event,
                             unsigned int fbo);

#ifdef __cplusplus
}
#endif
#endif  // PLAYER_H
