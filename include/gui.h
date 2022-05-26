#pragma once
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "player.h"

struct GuiData {
  float mouse_radius;
  float pixelization;
  bool mouse_debug;
};

class Gui {
 public:
  Gui(window_ctx& window_ctx);
  bool process_event(SDL_Event& event);
  void render(struct GuiData& data);
};
