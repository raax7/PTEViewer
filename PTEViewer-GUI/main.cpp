#include <iostream>
#include <imgui.h>

#include "GUI.h"
#include "Globals.h"
#include "Config.h"

#include "PollManager.h"

#include "ProcessListUI.h"
#include "PTViewerUI.h"

void DrawCallback();

int main()
{
    Globals::Setup();

    GUI* gui = GUI::CreateGUI();
    if (gui == nullptr)
    {
        std::cerr << "Failed to create GUI!" << std::endl;
        return 1;
    }

    // Render will run until the application is requested to close.
    //
    gui->Render(DrawCallback);

    gui->DestroyGUI();
    return 0;
}

void DrawCallback()
{
    Globals::GetPollManager()->PollAll();

    ImGui::Begin("Main Window");
    if (ImGui::BeginTabBar("Main Tab Bar"))
    {
        if (ImGui::BeginTabItem("Process List"))
        {
            Globals::GetProcessListUI()->Draw();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Config"))
        {
            ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
            ImGui::SameLine();
            ImGui::Checkbox("Show Demo Window", &Config::cfg_ShowDemoWindow);
            if (Config::cfg_ShowDemoWindow)
            {
                ImGui::ShowDemoWindow();
            }

            ImGui::SliderInt("Process Poll Interval (ms)", &Config::cfg_ProcessPollIntervalMs, 0, 10000);
            ImGui::SliderInt("Page-Table Poll Interval (ms)", &Config::cfg_PageTablePollIntervalMs, 0, 10000);
            ImGui::SliderInt("Process Close Stick Time (ms)", &Config::cfg_ClosedProcessStickTimeMs, 0, 10000);
            ImGui::SliderInt("Process Create Stick Time (ms)", &Config::cfg_NewProcessStickTimeMs, 0, 10000);

            ImGui::EndTabItem();
        }
    }
    ImGui::End();

    for (int i = 0; i < Globals::g_InspectedPTs.size(); i++)
    {
        const auto& PT = Globals::g_InspectedPTs[i];
        bool Open = true;

        if (ImGui::Begin(std::string("PT-Inspector (" + std::to_string(i) + ")").c_str(), &Open))
        {
            PT->Draw();
            ImGui::End();
        }

        if (!Open)
        {
            Globals::g_InspectedPTs.erase(Globals::g_InspectedPTs.begin() + i);
        }
    }
}
