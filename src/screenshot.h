#pragma once
#include <filesystem>

// Saves the currently bound framebuffer (call before swapBuffers, so it holds the finished frame) as a PNG.
// Reads at framebuffer resolution, which on a Retina screen is twice the window size.
// Returns false if the file could not be written.
bool saveScreenshot(const std::filesystem::path& filePath, int width, int height);

// Returns "<folder>/screenshot_NNN.png" with the first number that is not taken yet.
// A counter instead of a timestamp keeps this portable without relying on chrono formatting support.
std::filesystem::path nextFreeScreenshotPath(const std::filesystem::path& folder);
