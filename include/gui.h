#pragma once
#include <string>
#include <vector>

#include "config.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "player.h"

namespace sendprotest {
using namespace std;

class Gui {
 public:
  Gui(window_ctx& window_ctx);
  bool process_event(SDL_Event& event);
  void render(Config& config);
};
}  // namespace sendprotest
