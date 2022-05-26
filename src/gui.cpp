#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include "gui.h"

Gui::Gui(window_ctx& window_ctx) {
  const char* glsl_version = "#version 130";
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window_ctx.window, window_ctx.gl);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

void Gui::process_event(SDL_Event& event) {
  ImGui_ImplSDL2_ProcessEvent(&event);
}

void Gui::render(float& pixelization) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  {
    ImGui::Begin("Settings");
    ImGui::SliderFloat("Pixelization", &pixelization, 1.0f, 100.0f);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  // Rendering
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
