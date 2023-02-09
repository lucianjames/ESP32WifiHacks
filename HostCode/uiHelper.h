/*
    File copypasted from TorPlusPlus
*/

#pragma once

#include <imgui.h>

namespace uiHelper{

/*
    Set the size and position of the next window using normalised screen coordinates
*/
void setNextWindowSizeNormalised(float wStartXNorm,
                                 float wStartYNorm,
                                 float wEndXNorm,
                                 float wEndYNorm,
                                 ImGuiCond condition=ImGuiCond_Always){
        int wStartX = wStartXNorm * ImGui::GetIO().DisplaySize.x;
        int wStartY = wStartYNorm * ImGui::GetIO().DisplaySize.y;
        int wWidth = (wEndXNorm * ImGui::GetIO().DisplaySize.x) - wStartX;
        int wHeight = (wEndYNorm * ImGui::GetIO().DisplaySize.y) - wStartY;
        ImGui::SetNextWindowPos(ImVec2(wStartX, wStartY), condition);
        ImGui::SetNextWindowSize(ImVec2(wWidth, wHeight), condition);
}

}; // namespace uiHelper