#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"

int main() {
    // ImTui setup:
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImTui::TScreen* screen = ImTui_ImplNcurses_Init(true);
    ImTui_ImplText_Init();
    while(!ImGui::IsKeyPressed(27)){ // Exit cleanly if esc pressed
        // Start the frame
        ImTui_ImplNcurses_NewFrame();
        ImTui_ImplText_NewFrame();
        ImGui::NewFrame();
        
        /* ... All the important stuff happens here ... */
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::Begin("Hello world");
        ImGui::Text("World hello");
        ImGui::End();

        // Render the frame to the terminal
        ImGui::Render();
        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
        ImTui_ImplNcurses_DrawScreen();
    }
    // Cleanup
    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
    return 0;
}