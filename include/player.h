#ifndef PLAYER_H
#define PLAYER_H

#include <glad/glad.h>
// clang-format off
// must be included after glad
#include <SDL.h>
// clang-format on

void player_create();
void player_free();

void player_cmd(const char *cmd[]);
int player_draw(SDL_Event event, unsigned int fbo);
void player_swap_window();
#endif  // PLAYER_H
