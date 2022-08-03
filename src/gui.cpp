#include "gui.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

namespace sendprotest {
Gui::Gui(window_ctx& window_ctx) {
  const char* glsl_version = "#version 300 es";
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window_ctx.window, window_ctx.gl);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

bool Gui::process_event(SDL_Event& event) {
  ImGui_ImplSDL2_ProcessEvent(&event);
  auto io = ImGui::GetIO();
  return io.WantCaptureMouse;
}

void Gui::render(Config& config) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  auto& config_map = config.get_effects();

  if (ImGui::Begin("config")) {
    ImGui::ListBoxHeader("Effect", config_map.size());
    auto selected_name = config.get_selected_name();
    for (auto const& kv : config_map) {
      auto& effect_name = kv.first;
      bool is_selected = effect_name == selected_name;
      if (ImGui::Selectable(effect_name.c_str(), is_selected)) {
        config.set_selected_name(effect_name);
        selected_name = effect_name;
      }
    }
    ImGui::ListBoxFooter();

    auto& effect_config = config.get_selected_effect();
    auto& player_config = config.get_player_config();

    ImGui::SliderFloat("Effect strength", &effect_config.effect_amount, 0.0f,
                       100.0f);
    ImGui::SliderFloat("Touch Radius", &effect_config.finger_radius, 0.0f,
                       1.0f);
    ImGui::SliderFloat("Touch Fade In", &effect_config.effect_fade_in, 0.0f,
                       1.0f);
    ImGui::SliderInt("Playback Duration", &player_config.playback_duration, 1,
                     180);
    ImGui::SliderInt("Category", &player_config.category, 0, 8);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    if (ImGui::Button("Save Config")) {
      config.save();
    }
  }
  ImGui::End();
  // Rendering
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
}  // namespace sendprotest
