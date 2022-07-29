#pragma once
#include <string>
#include <vector>

#include "config.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "player.h"

namespace mpv_glsl {
using namespace std;

class Gui {
 public:
  Gui(window_ctx& window_ctx);
  bool process_event(SDL_Event& event);
  void render(Config& config, int& playback_duration);
};
}  // namespace mpv_glsl
