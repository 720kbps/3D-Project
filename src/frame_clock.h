#pragma once

// Turns wall-clock timestamps into the times every animation needs:
//  - realDelta: how long the last frame took (FPS readout, UI smoothing),
//  - simTime / simDelta: a pausable, scalable "world" clock that all animation reads.
// Animating in seconds instead of frames keeps motion at the same speed on every machine,
// which the brief checks explicitly (e.g. animated textures must be framerate independent).
class FrameClock {
public:
    // Call exactly once per frame with the current time in seconds (e.g. glfwGetTime()).
    // While paused the simulation clock stands still; timeScale speeds it up (> 1) or slows it down (< 1).
    void tick(double nowSeconds, bool paused, float timeScale);

    [[nodiscard]] float realDelta() const { return m_realDelta; }
    [[nodiscard]] float simDelta() const { return m_simDelta; }
    [[nodiscard]] double simTime() const { return m_simTime; }
    // Averaged over FPS_WINDOW seconds so the number is readable instead of flickering every frame.
    [[nodiscard]] float averageFps() const { return m_averageFps; }
    [[nodiscard]] float averageFrameMs() const { return m_averageFrameMs; }

private:
    // A breakpoint or a window drag can stall the loop for seconds. Without a cap the next frame
    // would teleport every animated object; 0.1 s keeps motion sane down to 10 FPS.
    static constexpr float MAX_DELTA = 0.1f;
    static constexpr double FPS_WINDOW = 0.5;

    bool m_started { false };
    double m_lastTime { 0.0 };
    float m_realDelta { 0.0f };
    float m_simDelta { 0.0f };
    // double, not float: a float clock loses millisecond precision after a few hours of running.
    double m_simTime { 0.0 };

    int m_framesInWindow { 0 };
    double m_windowTime { 0.0 };
    float m_averageFps { 0.0f };
    float m_averageFrameMs { 0.0f };
};
