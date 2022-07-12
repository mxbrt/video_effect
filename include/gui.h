#pragma once
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "player.h"

namespace mpv_glsl {
using namespace std;

struct EffectItem {
  const char* name;
  bool is_selected;
};

struct GuiData {
  float finger_radius;
  float effect_fade_in;
  float effect_fade_out;
  float effect_amount;
  vector<EffectItem> effects;
  size_t selected_effect;
};

class Gui {
 public:
  Gui(window_ctx& window_ctx);
  bool process_event(SDL_Event& event);
  void render(struct GuiData& data);
};
}  // namespace mpv_glsl
