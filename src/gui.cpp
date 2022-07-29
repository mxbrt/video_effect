#include "gui.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

namespace mpv_glsl {
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

void Gui::render(Config& config, int& playback_duration) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  auto& config_map = config.get();

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

    auto& data = config.get_selected_effect();

    ImGui::SliderFloat("Effect strength", &data.effect_amount, 0.0f, 100.0f);
    ImGui::SliderFloat("Touch Radius", &data.finger_radius, 0.0f, 1.0f);
    ImGui::SliderFloat("Touch Fade In", &data.effect_fade_in, 0.0f, 1.0f);
    ImGui::SliderInt("Playback Duration", &playback_duration, 1, 180);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  }
  ImGui::End();
  // Rendering
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
}  // namespace mpv_glsl
