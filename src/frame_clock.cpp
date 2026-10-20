#include "frame_clock.h"
#include <algorithm>

void FrameClock::tick(double nowSeconds, bool paused, float timeScale)
{
    // The first frame has no previous timestamp, so treat it as a zero-length frame.
    if (!m_started) {
        m_lastTime = nowSeconds;
        m_started = true;
    }
    const double elapsed = nowSeconds - m_lastTime;
    m_lastTime = nowSeconds;

    m_realDelta = std::min(static_cast<float>(elapsed), MAX_DELTA);
    m_simDelta = paused ? 0.0f : m_realDelta * timeScale;
    m_simTime += static_cast<double>(m_simDelta);

    // The FPS readout uses the unclamped time so it stays honest when frames are slow.
    m_framesInWindow++;
    m_windowTime += elapsed;
    if (m_windowTime >= FPS_WINDOW) {
        m_averageFps = static_cast<float>(m_framesInWindow / m_windowTime);
        m_averageFrameMs = static_cast<float>(1000.0 * m_windowTime / m_framesInWindow);
        m_framesInWindow = 0;
        m_windowTime = 0.0;
    }
}
