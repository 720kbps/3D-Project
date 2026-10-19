#include "ui.h"
#include "frame_clock.h"
#include "scene.h"
#include "settings.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()

namespace {
// Frame timing. To check framerate independence, raise "Simulated lag" or turn VSync off:
// the FPS changes, but the spin test must keep doing one turn every 8 seconds.
void drawFrameSection(Settings& settings, const FrameClock& clock)
{
    if (!ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    // ImGui::Text is printf-style (varargs), so floats are passed as double explicitly.
    ImGui::Text("%.1f FPS (%.2f ms)", static_cast<double>(clock.averageFps()), static_cast<double>(clock.averageFrameMs()));
    ImGui::Text("Sim time: %.2f s", clock.simTime());
    ImGui::Checkbox("VSync", &settings.vsync);
    ImGui::Checkbox("Pause", &settings.paused);
    ImGui::SliderFloat("Time scale", &settings.timeScale, 0.0f, 4.0f);
    ImGui::SliderInt("Simulated lag (ms)", &settings.simulatedLagMs, 0, 100);
    ImGui::Checkbox("Spin test", &settings.spinTest);
}

void drawDebugSection(Settings& settings)
{
    if (!ImGui::CollapsingHeader("Debug"))
        return;
    // Order must match enum class DebugView.
    int view = static_cast<int>(settings.debugView);
    if (ImGui::Combo("View", &view, "Default\0Normals\0UVs\0Depth\0Cubemap (debug)\0Tangents\0Tangent sign\0"))
        settings.debugView = static_cast<DebugView>(view);
    // Order must match enum class BufferView.
    int buffer = static_cast<int>(settings.bufferView);
    if (ImGui::Combo("Buffer", &buffer, "Final image\0Normal buffer\0Depth buffer\0"))
        settings.bufferView = static_cast<BufferView>(buffer);
    if (settings.debugView == DebugView::Depth || settings.bufferView == BufferView::Depth)
        ImGui::SliderFloat("Depth range", &settings.depthViewRange, 0.5f, 20.0f);
    ImGui::TextUnformatted("F12: screenshot to docs/screens/");
}

// Final pass. With the scene still unlit, set the tonemapper to Reinhard or ACES and raise the exposure
// to see highlights roll off instead of clipping to white.
void drawPostSection(Settings& settings)
{
    if (!ImGui::CollapsingHeader("Post-processing"))
        return;
    ImGui::SliderFloat("Exposure (EV)", &settings.exposureEV, -4.0f, 4.0f, "%+.1f");
    // Order must match enum class Tonemapper.
    int tonemapper = static_cast<int>(settings.tonemapper);
    if (ImGui::Combo("Tonemapper", &tonemapper, "None (clamp)\0Reinhard\0ACES (fitted)\0"))
        settings.tonemapper = static_cast<Tonemapper>(tonemapper);
    ImGui::Checkbox("sRGB encode", &settings.srgbEncode);
}

// Show or hide individual objects, e.g. to isolate one feature for a screenshot.
void drawSceneSection(Scene& scene)
{
    if (!ImGui::CollapsingHeader("Scene"))
        return;
    ImGui::TextUnformatted("Camera: left drag orbits, right drag pans, scroll zooms.");
    for (Entity& entity : scene.entities())
        ImGui::Checkbox(entity.name.c_str(), &entity.visible);
}
}

void drawSettingsUI(Settings& settings, const FrameClock& clock, Scene& scene)
{
    ImGui::Begin("Settings");
    drawFrameSection(settings, clock);
    drawPostSection(settings);
    drawDebugSection(settings);
    drawSceneSection(scene);
    ImGui::End();
}
