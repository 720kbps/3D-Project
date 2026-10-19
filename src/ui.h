#pragma once

struct Settings;
class FrameClock;
class Scene;

// Draws the settings panel: one collapsible section per feature. Each new feature adds its toggle and
// the one to three parameters that make it easy to demonstrate.
void drawSettingsUI(Settings& settings, const FrameClock& clock, Scene& scene);
