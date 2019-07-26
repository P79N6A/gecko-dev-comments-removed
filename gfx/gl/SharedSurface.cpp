




#include "SharedSurface.h"
#include "SharedSurfaceGL.h"

using namespace mozilla::gl;

namespace mozilla {
namespace gfx {



void
SharedSurface::Copy(SharedSurface* src, SharedSurface* dest, SurfaceFactory* factory)
{
    MOZ_ASSERT( src->APIType() == APITypeT::OpenGL);
    MOZ_ASSERT(dest->APIType() == APITypeT::OpenGL);

    SharedSurface_GL* srcGL = (SharedSurface_GL*)src;
    SharedSurface_GL* destGL = (SharedSurface_GL*)dest;

    SharedSurface_GL::Copy(srcGL, destGL, (SurfaceFactory_GL*)factory);
}

} 
} 
