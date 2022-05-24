#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sdl_gl.h"

void player_create();
void player_free();

void player_cmd(const char *cmd[]);
int player_draw(SDL_Event event, unsigned int fbo);
void player_swap_window();

#ifdef __cplusplus
}
#endif
#endif  // PLAYER_H
