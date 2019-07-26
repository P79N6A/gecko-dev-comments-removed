





#ifndef GLCONTEXTUTILS_H_
#define GLCONTEXTUTILS_H_

#include "GLContextTypes.h"
#include "mozilla/gfx/Types.h"

namespace mozilla {
namespace gfx {
  class DataSourceSurface;
}
}

namespace mozilla {
namespace gl {

TemporaryRef<gfx::DataSourceSurface>
ReadBackSurface(GLContext* aContext, GLuint aTexture, bool aYInvert, gfx::SurfaceFormat aFormat);

} 
} 

#endif 
