




#include "GLContextTypes.h"
#include <cstring>

using namespace mozilla::gl;

GLFormats::GLFormats()
{
    std::memset(this, 0, sizeof(GLFormats));
}

PixelBufferFormat::PixelBufferFormat()
{
    std::memset(this, 0, sizeof(PixelBufferFormat));
}
