#include "screenshot.h"
#include <framework/opengl_includes.h>
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <stb/stb_image_write.h> // The implementation is compiled into the framework (image.cpp).
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cstdio>
#include <vector>

bool saveScreenshot(const std::filesystem::path& filePath, int width, int height)
{
    if (width <= 0 || height <= 0)
        return false;
    const size_t rowBytes = 3 * static_cast<size_t>(width);
    std::vector<unsigned char> pixels(rowBytes * static_cast<size_t>(height));

    // RGB rows are not always a multiple of 4 bytes, so tell OpenGL to pack them tightly.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL's first row is the bottom of the image; PNG's first row is the top.
    for (int row = 0; row < height / 2; ++row) {
        auto top = pixels.begin() + static_cast<std::ptrdiff_t>(rowBytes) * row;
        auto bottom = pixels.begin() + static_cast<std::ptrdiff_t>(rowBytes) * (height - 1 - row);
        std::swap_ranges(top, top + static_cast<std::ptrdiff_t>(rowBytes), bottom);
    }

    std::filesystem::create_directories(filePath.parent_path());
    return stbi_write_png(filePath.string().c_str(), width, height, 3, pixels.data(), static_cast<int>(rowBytes)) != 0;
}

std::filesystem::path nextFreeScreenshotPath(const std::filesystem::path& folder)
{
    for (int i = 1;; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "screenshot_%03d.png", i);
        const std::filesystem::path candidate = folder / name;
        if (!std::filesystem::exists(candidate))
            return candidate;
    }
}
