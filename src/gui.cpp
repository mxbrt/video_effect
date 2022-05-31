#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include "gui.h"

namespace mpv_glsl {
Gui::Gui(window_ctx& window_ctx) {
  const char* glsl_version = "#version 130";
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
    ImGui::SliderFloat("Pixelization", &data.pixelization, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Radius", &data.mouse_radius, 0.0f, 1.0f);
    ImGui::SliderFloat("Mouse Fade In", &data.mouse_fade_in, 0.0f, 1.0f);
    ImGui::SliderFloat("Mouse Fade Out", &data.mouse_fade_out, 0.0f, 1.0f);
    ImGui::Checkbox("Mouse Debug", &data.mouse_debug);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  // Rendering
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
}  // namespace mpv_glsl
