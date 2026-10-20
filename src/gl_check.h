#pragma once
#include <source_location>
#include <string_view>

// Prints every pending OpenGL error with a label and the call site, then leaves the error queue empty.
// OpenGL 4.1 (the macOS maximum) has no glDebugMessageCallback, so calling this after each render pass
// is how we find out which pass broke. Does nothing in Release builds (NDEBUG).
void checkGLErrors(std::string_view label, std::source_location where = std::source_location::current());
