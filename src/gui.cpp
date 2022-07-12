#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include "gui.h"

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

void Gui::render(struct GuiData& data) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  {
    ImGui::Begin("Settings");
    ImGui::SliderFloat("Effect strength", &data.effect_amount, 0.0f, 100.0f);
    ImGui::SliderFloat("Touch Radius", &data.finger_radius, 0.0f, 1.0f);
    ImGui::SliderFloat("Effect Fade In", &data.effect_fade_in, 0.0f, 1.0f);
    ImGui::SliderFloat("Effect Fade Out", &data.effect_fade_out, 0.0f, 1.0f);
    ImGui::Checkbox("Touch Debug", &data.input_debug);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  // Rendering
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
}  // namespace mpv_glsl
