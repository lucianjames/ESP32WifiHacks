#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"

#include "interface.h"

int main(int argc, char** argv){
    if(argc < 2){
        std::cout << "Usage: " << argv[0] << " <serial port>" << std::endl;
        return 1;
    }

    // ImTui setup:
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImTui::TScreen* screen = ImTui_ImplNcurses_Init(true);
    ImTui_ImplText_Init();

    ESP32Interface UI(argv[1]);

    int frames = 0;
    while(!ImGui::IsKeyPressed(27)){ // Exit cleanly if esc pressed
        // Start the frame
        ImTui_ImplNcurses_NewFrame();
        ImTui_ImplText_NewFrame();
        ImGui::NewFrame();

        UI.update();

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