#pragma once

// What the main fragment shader outputs. Sent to the shader as an int, so the values must match the
// DEBUG_* constants in shaders/shader_frag.glsl.
enum class DebugView : int {
    Default = 0, // Normal shading.
    Normals = 1, // World-space normal remapped from [-1, 1] to [0, 1].
    TexCoords = 2, // fract(UV) as red/green, so tiled UVs show as repeating gradients.
    Depth = 3, // Distance to the camera: white is near, black is depthViewRange or further.
    Cubemap = 4, // The debug cubemap sampled along the normal; must look identical to Normals if cubemap loading is correct.
    Tangents = 5, // World-space tangent (direction of increasing u) remapped to [0, 1].
    TangentSign = 6, // Tangent handedness: green where w = +1, red where the UVs are mirrored (w = -1).
};

// How the final pass compresses HDR values into [0, 1]. Values must match TONEMAP_* in shaders/tonemap_frag.glsl.
enum class Tonemapper : int {
    None = 0, // Clamp only: anything above 1 clips to white.
    Reinhard = 1, // c / (1 + c): simple, but greys out bright saturated colours.
    Aces = 2, // Filmic S-curve fit (Narkowicz): more contrast, highlights roll off smoothly.
};

// Which offscreen buffer the final pass shows. Values must match BUFFER_* in shaders/tonemap_frag.glsl.
enum class BufferView : int {
    Final = 0, // The tonemapped image.
    Normals = 1, // View-space normals stored by the main pass.
    Depth = 2, // Depth buffer, linearised to a distance; white is near.
};

// Every feature toggle and parameter lives here, so the UI and the render code read one place.
// Rule from the brief: every feature must be toggleable from the UI, so nothing is on without a field here.
struct Settings {
    // Frame
    bool vsync { true }; // The framework enables vsync when it creates the window.
    bool paused { false };
    float timeScale { 1.0f };
    int simulatedLagMs { 0 };
    bool spinTest { true };

    // Debug
    DebugView debugView { DebugView::Default };
    float depthViewRange { 10.0f }; // About twice the camera's start distance, so the whole test scene is readable.

    // Post-processing (final pass)
    float exposureEV { 0.0f }; // In stops: each +1 doubles the brightness.
    // None while the scene is unlit: flat colours are already in [0, 1], and a curve would only dim them.
    // Switch the default once lighting produces HDR values.
    Tonemapper tonemapper { Tonemapper::None };
    bool srgbEncode { true }; // Off shows the raw linear values, which look too dark: useful to see why the step exists.
    BufferView bufferView { BufferView::Final };
};
