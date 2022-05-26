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

void player_create(struct window_ctx *w);
void player_free();

void player_cmd(const char *cmd[]);
int player_draw(struct window_ctx *w, SDL_Event event, unsigned int fbo);

#ifdef __cplusplus
}
#endif
#endif  // PLAYER_H
