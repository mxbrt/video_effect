#pragma once
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "player.h"

namespace mpv_glsl {
struct GuiData {
  float finger_radius;
  float effect_fade_in;
  float effect_fade_out;
  float effect_amount;
  bool input_debug;
};

class Gui {
 public:
  Gui(window_ctx& window_ctx);
  bool process_event(SDL_Event& event);
  void render(struct GuiData& data);
};
}  // namespace mpv_glsl
