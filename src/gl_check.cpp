#include "gl_check.h"
#include <framework/opengl_includes.h>
#include <iostream>

namespace {
const char* glErrorName(GLenum error)
{
    switch (error) {
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "unknown GL error";
    }
}
}

void checkGLErrors([[maybe_unused]] std::string_view label, [[maybe_unused]] std::source_location where)
{
#ifndef NDEBUG
    // Only print the file name, not the full path, to keep the line short.
    std::string_view file = where.file_name();
    file = file.substr(file.find_last_of("/\\") + 1);

    // glGetError returns one error flag per call, so keep reading until the queue is empty.
    // The cap avoids an endless loop if the context is lost (some drivers then report errors forever).
    for (int i = 0; i < 16; ++i) {
        const GLenum error = glGetError();
        if (error == GL_NO_ERROR)
            return;
        std::cerr << "[GL] " << glErrorName(error) << " after '" << label << "' (" << file << ":" << where.line() << ")\n";
    }
#endif
}
