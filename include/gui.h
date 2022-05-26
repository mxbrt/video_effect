#pragma once
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "player.h"

class Gui {
 public:
  Gui(window_ctx& window_ctx);
  bool process_event(SDL_Event& event);
  void render(float& pixelization);
};
